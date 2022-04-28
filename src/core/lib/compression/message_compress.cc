/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <grpc/support/port_platform.h>

#include "src/core/lib/compression/message_compress.h"

#include <string.h>
#include <stdio.h>
#include <zlib.h>
#include <lz4.h>
#include <lz4frame.h>
#include <grpc/support/alloc.h>
#include <grpc/support/log.h>

#include "src/core/lib/slice/slice_internal.h"

#define OUTPUT_BLOCK_SIZE 1024
#define IN_CHUNK_SIZE  (16*1024)

static int zlib_body(z_stream* zs, grpc_slice_buffer* input,
                     grpc_slice_buffer* output,
                     int (*flate)(z_stream* zs, int flush)) {
  int r = Z_STREAM_END; /* Do not fail on an empty input. */
  int flush;
  size_t i;
  grpc_slice outbuf = GRPC_SLICE_MALLOC(OUTPUT_BLOCK_SIZE);
  const uInt uint_max = ~static_cast<uInt>(0);

  GPR_ASSERT(GRPC_SLICE_LENGTH(outbuf) <= uint_max);
  zs->avail_out = static_cast<uInt> GRPC_SLICE_LENGTH(outbuf);
  zs->next_out = GRPC_SLICE_START_PTR(outbuf);
  flush = Z_NO_FLUSH;
  for (i = 0; i < input->count; i++) {
    if (i == input->count - 1) flush = Z_FINISH;
    GPR_ASSERT(GRPC_SLICE_LENGTH(input->slices[i]) <= uint_max);
    zs->avail_in = static_cast<uInt> GRPC_SLICE_LENGTH(input->slices[i]);
    zs->next_in = GRPC_SLICE_START_PTR(input->slices[i]);
    do {
      if (zs->avail_out == 0) {
        grpc_slice_buffer_add_indexed(output, outbuf);
        outbuf = GRPC_SLICE_MALLOC(OUTPUT_BLOCK_SIZE);
        GPR_ASSERT(GRPC_SLICE_LENGTH(outbuf) <= uint_max);
        zs->avail_out = static_cast<uInt> GRPC_SLICE_LENGTH(outbuf);
        zs->next_out = GRPC_SLICE_START_PTR(outbuf);
      }
      r = flate(zs, flush);
      if (r < 0 && r != Z_BUF_ERROR /* not fatal */) {
        gpr_log(GPR_INFO, "zlib error (%d)", r);
        goto error;
      }
    } while (zs->avail_out == 0);
    if (zs->avail_in) {
      gpr_log(GPR_INFO, "zlib: not all input consumed");
      goto error;
    }
  }
  if (r != Z_STREAM_END) {
    gpr_log(GPR_INFO, "zlib: Data error");
    goto error;
  }

  GPR_ASSERT(outbuf.refcount);
  outbuf.data.refcounted.length -= zs->avail_out;
  grpc_slice_buffer_add_indexed(output, outbuf);

  return 1;

error:
  grpc_slice_unref_internal(outbuf);
  return 0;
}

static void* zalloc_gpr(void* /*opaque*/, unsigned int items,
                        unsigned int size) {
  return gpr_malloc(items * size);
}

static void zfree_gpr(void* /*opaque*/, void* address) { gpr_free(address); }

static int zlib_compress(grpc_slice_buffer* input, grpc_slice_buffer* output,
                         int gzip) {
  z_stream zs;
  int r;
  size_t i;
  size_t count_before = output->count;
  size_t length_before = output->length;
  memset(&zs, 0, sizeof(zs));
  zs.zalloc = zalloc_gpr;
  zs.zfree = zfree_gpr;
  r = deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | (gzip ? 16 : 0),
                   8, Z_DEFAULT_STRATEGY);
  GPR_ASSERT(r == Z_OK);
  r = zlib_body(&zs, input, output, deflate) && output->length < input->length;
  if (!r) {
    for (i = count_before; i < output->count; i++) {
      grpc_slice_unref_internal(output->slices[i]);
    }
    output->count = count_before;
    output->length = length_before;
  }
  deflateEnd(&zs);
  return r;
}

static int zlib_decompress(grpc_slice_buffer* input, grpc_slice_buffer* output,
                           int gzip) {
  z_stream zs;
  int r;
  size_t i;
  size_t count_before = output->count;
  size_t length_before = output->length;
  memset(&zs, 0, sizeof(zs));
  zs.zalloc = zalloc_gpr;
  zs.zfree = zfree_gpr;
  r = inflateInit2(&zs, 15 | (gzip ? 16 : 0));
  GPR_ASSERT(r == Z_OK);
  r = zlib_body(&zs, input, output, inflate);
  if (!r) {
    for (i = count_before; i < output->count; i++) {
      grpc_slice_unref_internal(output->slices[i]);
    }
    output->count = count_before;
    output->length = length_before;
  }
  inflateEnd(&zs);
  return r;
}

static const LZ4F_preferences_t kPrefs = {
  { 
    LZ4F_max256KB, 
    LZ4F_blockLinked, 
    LZ4F_noContentChecksum, 
    LZ4F_frame,
    0, 
    LZ4F_noBlockChecksum 
  },
  0,   
  0, 
  0,  
  { 0, 0, 0 },
};

// std::tuple<int, int> read_grpc_slice_buffer(grpc_slice_buffer* input, void* buffer, size_t bufferSz, size_t offset) {
//   if( input == NULL){
//     return std::tuple<int, int>{-1, -1};
//   }

//   if( offset >= input->length) {
//     return std::tuple<int, int>{0, 0};
//   }

//   //find first block
//   size_t visitedSliceSize = 0;
//   size_t inputIndex = 0;
//   for(int i = 0 ; i < input->count; i++) {
//     if( visitedSliceSize + GRPC_SLICE_LENGTH( input->slices[i] ) > offset ) {
//       inputIndex = i;
//       break;
//     } else {
//       visitedSliceSize = visitedSliceSize + GRPC_SLICE_LENGTH( input->slices[i] );
//     }
//   }
  
//   auto startIndex = offset - visitedSliceSize;
//   // 处理表头
//   if( startIndex + bufferSz <= GRPC_SLICE_LENGTH( input->slices[inputIndex] ) ) {
      
//   }

//   // size_t readedSize = 0;
//   // while( readedSize < bufferSz ) {
//   //   strncpy((char*) buffer, buffer, cmpBytes);
//   // }
// }

static int lz4_compress(grpc_slice_buffer* input, grpc_slice_buffer* output) {

  LZ4F_compressionContext_t ctx;
  size_t const ctxCreation = LZ4F_createCompressionContext(&ctx, LZ4F_VERSION);
  size_t const outCapacity = LZ4F_compressBound(IN_CHUNK_SIZE, &kPrefs);

  char* srcBuffer = reinterpret_cast<char*>(malloc(input->length));
  if (!srcBuffer) { 
    printf("decompress_file(src)"); 
    free(srcBuffer);
    return 1; 
  }

  size_t copiedSz = 0;
  for(int i =0 ; i < input->count; i++) {
    auto sgSliceSZ = GRPC_SLICE_LENGTH( input->slices[i] );
    memcpy(srcBuffer + copiedSz,  reinterpret_cast<char*> GRPC_SLICE_START_PTR( input->slices[i] ), sgSliceSZ);
  }
  
  // process lz4 frame header
  { 
    size_t const outbufCapacity = LZ4F_compressBound(IN_CHUNK_SIZE, &kPrefs); 
    char* headerBuff = reinterpret_cast<char*>( malloc(outbufCapacity) ) ;
    size_t const headerSize = LZ4F_compressBegin(ctx, headerBuff, outCapacity, &kPrefs);

    if (LZ4F_isError(headerSize)) {
      printf("Failed to start compression: error %u \n", (unsigned)headerSize);
      free(srcBuffer);
      return 1;
    }

    auto count_out = headerSize;
    printf("Buffer size is %u bytes, header size %u bytes \n",
                (unsigned)outCapacity, (unsigned)headerSize);

    grpc_slice outbuf = GRPC_SLICE_MALLOC(headerSize);
    char* headerBufferPtr = reinterpret_cast<char*> GRPC_SLICE_START_PTR(outbuf);
    strncpy(headerBufferPtr, headerBuff, headerSize);
    grpc_slice_buffer_add_indexed(output, outbuf);
  }


  // // process lz4 frame header
  // { 
  //   size_t const outbufCapacity = LZ4F_compressBound(IN_CHUNK_SIZE, &kPrefs); 
  //   char* headerBuff = reinterpret_cast<char*>( malloc(outbufCapacity) ) ;
  //   size_t const headerSize = LZ4F_compressBegin(ctx, headerBuff, outCapacity, &kPrefs);

  //   if (LZ4F_isError(headerSize)) {
  //     printf("Failed to start compression: error %u \n", (unsigned)headerSize);
  //     return 1;
  //   }

  //   auto count_out = headerSize;
  //   printf("Buffer size is %u bytes, header size %u bytes \n",
  //               (unsigned)outCapacity, (unsigned)headerSize);

  //   grpc_slice outbuf = GRPC_SLICE_MALLOC(headerSize);
  //   char* headerBufferPtr = reinterpret_cast<char*> GRPC_SLICE_START_PTR(outbuf);
  //   strncpy(headerBufferPtr, headerBuff, headerSize);
  //   grpc_slice_buffer_add_indexed(output, outbuf);

  // }
  
  // const uInt uint_max = ~static_cast<uInt>(0);
  // char buffer[ OUTPUT_BLOCK_SIZE * 16 ];
  // for (auto i = 0 ; i < input->count ; i++ ) {
    
  //   GPR_ASSERT(GRPC_SLICE_LENGTH(input->slices[i]) <= uint_max);
  //   uint32_t inputSz = GRPC_SLICE_LENGTH( input->slices[i] );
  //   const char* inpPtr = reinterpret_cast<char*> GRPC_SLICE_START_PTR( input->slices[i] );

  //   size_t const compressedSize = LZ4F_compressUpdate(ctx,
  //                                               buffer, outCapacity,
  //                                               inpPtr, inputSz,
  //                                               NULL);
  //   if (LZ4F_isError(compressedSize)) {
  //     printf("Compression failed: error %u \n", (unsigned)compressedSize);
  //     return 1;
  //   }
    
  //   grpc_slice outbuf = GRPC_SLICE_MALLOC(compressedSize);
  //   char* outBufferPtr = reinterpret_cast<char*> GRPC_SLICE_START_PTR(outbuf);
  //   strncpy(outBufferPtr, buffer, compressedSize);

  //   grpc_slice_buffer_add_indexed(output, outbuf);

  // }

  // {   
  //   size_t const compressedSize = LZ4F_compressEnd(ctx,
  //                                               buffer, outCapacity,
  //                                               NULL);
  //   if (LZ4F_isError(compressedSize)) {
  //     printf("Failed to end compression: error %u \n", (unsigned)compressedSize);
  //     return 1;
  //   }

  // }

  // return 0;

}

static int lz4_decompress(grpc_slice_buffer* input, grpc_slice_buffer* output) {

  size_t i;
  char buffer[ OUTPUT_BLOCK_SIZE * 16 ];
  const uInt uint_max = ~static_cast<uInt>(0);

  void* const src = malloc(IN_CHUNK_SIZE);
  if (!src) { 
    printf("decompress_file(src)"); 
    return 1; 
  }

  LZ4F_dctx* dctx;
  {   
    size_t const dctxStatus = LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);
    if (LZ4F_isError(dctxStatus)) {
      printf("LZ4F_dctx creation error: %s\n", LZ4F_getErrorName(dctxStatus));
    }   
  }

  free(src);
  LZ4F_freeDecompressionContext(dctx);

  return 0;
}

// static int lz4_compress(grpc_slice_buffer* input, grpc_slice_buffer* output) {
//   size_t i;
//   char buffer[ OUTPUT_BLOCK_SIZE * 16 ];
//   const uInt uint_max = ~static_cast<uInt>(0);
  
//   LZ4_stream_t lz4Stream_body;
//   LZ4_stream_t* lz4Stream = &lz4Stream_body;

//   uint32_t intput_size = 0;
//   uint32_t output_size = 0;
//   for ( i = 0; i < input->count; i++ ) {

//     GPR_ASSERT(GRPC_SLICE_LENGTH(input->slices[i]) <= uint_max);
//     uint32_t inputSz = GRPC_SLICE_LENGTH( input->slices[i] );
//     intput_size += inputSz;
//     const char* inpPtr = reinterpret_cast<char*> GRPC_SLICE_START_PTR( input->slices[i] );
//     const int cmpBytes = LZ4_compress_fast_continue(
//       lz4Stream, inpPtr, buffer, inputSz, sizeof(buffer), 1);


//     grpc_slice outbuf = GRPC_SLICE_MALLOC(cmpBytes);
//     char* outBufferPtr = reinterpret_cast<char*> GRPC_SLICE_START_PTR(outbuf);
//     strncpy(outBufferPtr, buffer, cmpBytes);

//     grpc_slice_buffer_add_indexed(output, outbuf);

//     output_size += cmpBytes;
//   }

//   return 0;
// }

// static int lz4_decompress(grpc_slice_buffer* input, grpc_slice_buffer* output) {
//   size_t i;
//   char buffer[ OUTPUT_BLOCK_SIZE * 16 ];
//   const uInt uint_max = ~static_cast<uInt>(0);
  
//   LZ4_streamDecode_t lz4StreamDecode_body;
//   LZ4_streamDecode_t* lz4StreamDecode = &lz4StreamDecode_body;

//   for ( i = 0; i < input->count; i++ ) {
    
//     GPR_ASSERT(GRPC_SLICE_LENGTH(input->slices[i]) <= uint_max);
//     uint32_t inputSz = GRPC_SLICE_LENGTH( input->slices[i] );

//     const char* inpPtr = reinterpret_cast<char*> GRPC_SLICE_START_PTR( input->slices[i] );
//     const int cmpBytes = LZ4_decompress_safe_continue(
//       lz4StreamDecode, inpPtr, buffer, inputSz, sizeof(buffer));


//     grpc_slice outbuf = GRPC_SLICE_MALLOC(cmpBytes);
//     char* outBufferPtr = reinterpret_cast<char*> GRPC_SLICE_START_PTR(outbuf);
//     if ( outBufferPtr == NULL ) {
//       std::cout << "outBufferPtr is NULL" << std::endl;
//     }
//     strncpy(outBufferPtr, buffer, cmpBytes);

//     grpc_slice_buffer_add_indexed(output, outbuf);
//   }

//   return 0;
// }

static int copy(grpc_slice_buffer* input, grpc_slice_buffer* output) {
  size_t i;
  for (i = 0; i < input->count; i++) {
    grpc_slice_buffer_add(output, grpc_slice_ref_internal(input->slices[i]));
  }
  return 1;
}

static int compress_inner(grpc_compression_algorithm algorithm,
                          grpc_slice_buffer* input, grpc_slice_buffer* output) {
  switch (algorithm) {
    case GRPC_COMPRESS_NONE:
      /* the fallback path always needs to be send uncompressed: we simply
         rely on that here */
      return 0;
    case GRPC_COMPRESS_DEFLATE:
      return zlib_compress(input, output, 0);
    case GRPC_COMPRESS_GZIP:
      return zlib_compress(input, output, 1);
    case GRPC_COMPRESS_LZ4:
      return lz4_compress(input, output);
    case GRPC_COMPRESS_ALGORITHMS_COUNT:
      break;
  }
  gpr_log(GPR_ERROR, "invalid compression algorithm %d", algorithm);
  return 0;
}

int grpc_msg_compress(grpc_compression_algorithm algorithm,
                      grpc_slice_buffer* input, grpc_slice_buffer* output) {
  if (!compress_inner(algorithm, input, output)) {
    copy(input, output);
    return 0;
  }
  return 1;
}

int grpc_msg_decompress(grpc_compression_algorithm algorithm,
                        grpc_slice_buffer* input, grpc_slice_buffer* output) {
  switch (algorithm) {
    case GRPC_COMPRESS_NONE:
      return copy(input, output);
    case GRPC_COMPRESS_DEFLATE:
      return zlib_decompress(input, output, 0);
    case GRPC_COMPRESS_GZIP:
      return zlib_decompress(input, output, 1);
    case GRPC_COMPRESS_LZ4:
      return lz4_decompress(input, output);
    case GRPC_COMPRESS_ALGORITHMS_COUNT:
      break;
  }
  gpr_log(GPR_ERROR, "invalid compression algorithm %d", algorithm);
  return 0;
}
