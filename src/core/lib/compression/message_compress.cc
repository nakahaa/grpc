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

#include <lz4.h>
#include <lz4frame.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <zlib.h>

#include <grpc/support/alloc.h>
#include <grpc/support/log.h>

#include "src/core/lib/slice/slice_internal.h"

#define OUTPUT_BLOCK_SIZE 1024
#define IN_CHUNK_SIZE (16 * 1024)

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
    {LZ4F_max256KB, LZ4F_blockLinked, LZ4F_noContentChecksum, LZ4F_frame, 0,
     LZ4F_noBlockChecksum},
    0,
    0,
    0,
    {0, 0, 0},
};

static size_t get_block_size(const LZ4F_frameInfo_t* info) {
  switch (info->blockSizeID) {
    case LZ4F_default:
    case LZ4F_max64KB:
      return 1 << 16;
    case LZ4F_max256KB:
      return 1 << 18;
    case LZ4F_max1MB:
      return 1 << 20;
    case LZ4F_max4MB:
      return 1 << 22;
    default:
      printf("Impossible with expected frame specification (<=v1.6.1)\n");
      return -1;
  }
}


typedef struct
{
    int error;
    unsigned long long size_in;
    unsigned long long size_out;
} compressResult_t;

static compressResult_t
compress_slice_internal(grpc_slice_buffer* input, grpc_slice_buffer* output,
                       LZ4F_compressionContext_t ctx,
                       void *outBuff, size_t outCapacity)
{
    compressResult_t result = {1, 0, 0};
    unsigned long long count_in = 0, count_out;
    const uInt uint_max = ~static_cast<uInt>(0);

    assert(input != NULL);
    assert(output != NULL);
    assert(ctx != NULL);
    assert(outCapacity >= LZ4F_HEADER_SIZE_MAX);

    // write frame header 
    {
        size_t const headerSize = LZ4F_compressBegin(ctx, outBuff, outCapacity, &kPrefs);
        if (LZ4F_isError(headerSize))
        {
            printf("Failed to start compression: error %u \n", (unsigned)headerSize);
            return result;
        }
        count_out = headerSize;
        printf("Buffer size is %u bytes, header size %u bytes \n",
               (unsigned)outCapacity, (unsigned)headerSize);

        grpc_slice header = GRPC_SLICE_MALLOC(headerSize);
        void* headerPtr = GRPC_SLICE_START_PTR(header);
        memcpy(headerPtr,  outBuff , headerSize);
        grpc_slice_buffer_add_indexed(output, header);
    }

    // stream
    for (size_t i =0 ; i < input->count ; i++)
    {   
        GPR_ASSERT(GRPC_SLICE_LENGTH(input->slices[i]) <= uint_max);
        size_t readSize = GRPC_SLICE_LENGTH( input->slices[i] );

        if (readSize == 0)
            break; 
        count_in += readSize;

        const void* inBuff = GRPC_SLICE_START_PTR( input->slices[i] );
        size_t const compressedSize = LZ4F_compressUpdate(ctx,
                                                          outBuff, outCapacity,
                                                          inBuff, readSize,
                                                          NULL);
        if (LZ4F_isError(compressedSize))
        {
            printf("Compression failed: error %u \n", (unsigned)compressedSize);
            return result;
        }

        if (compressedSize == 0 ){
          continue;
        }

        printf("Writing stream %u bytes\n", (unsigned)compressedSize);

        grpc_slice tmpOutbuf = GRPC_SLICE_MALLOC(compressedSize);
        void* outBufferPtr = GRPC_SLICE_START_PTR(tmpOutbuf);
        memcpy(outBufferPtr,  outBuff , compressedSize);
        grpc_slice_buffer_add_indexed(output, tmpOutbuf);

        count_out += compressedSize;
    }

    // flush whatever remains within internal buffers
    {
        size_t const compressedSize = LZ4F_compressEnd(ctx,
                                                       outBuff, outCapacity,
                                                       NULL);
        if (LZ4F_isError(compressedSize))
        {
            printf("Failed to end compression: error %u \n", (unsigned)compressedSize);
            return result;
        }
        
        grpc_slice tmpOutbuf = GRPC_SLICE_MALLOC(compressedSize);
        void* outBufferPtr = GRPC_SLICE_START_PTR(tmpOutbuf);
        memcpy(outBufferPtr,  outBuff , compressedSize);

        grpc_slice_buffer_add_indexed(output, tmpOutbuf);
        
        count_out += compressedSize;
    }

    result.size_in = count_in;
    result.size_out = count_out;
    result.error = 0;
    return result;
}

static int lz4_compress(grpc_slice_buffer* input, grpc_slice_buffer* output) {
  LZ4F_compressionContext_t ctx;
  size_t const ctxCreation = LZ4F_createCompressionContext(&ctx, LZ4F_VERSION);
  size_t maxBufferSz = 0;
  // std::cout << "before lz4 compress slices " << std::endl;
  for (size_t i = 0; i < input->count; i++) {
    // std::cout<< "slice = " << i << "," << "length = " << GRPC_SLICE_LENGTH( input->slices[i]) << std::endl;
    if ( maxBufferSz < GRPC_SLICE_LENGTH( input->slices[i]) ) {
      maxBufferSz = GRPC_SLICE_LENGTH( input->slices[i]);
    }
  }
  size_t const outbufCapacity = LZ4F_compressBound(maxBufferSz, &kPrefs);
  size_t const outCapacity = LZ4F_compressBound(maxBufferSz, &kPrefs);
  void *const src = malloc(outCapacity);
  void *const outbuff = malloc(outbufCapacity);
  if (!LZ4F_isError(ctxCreation) && src && outbuff)
  { 
      auto result = compress_slice_internal(input, output, ctx, outbuff, outbufCapacity);
  }
  else
  {
    printf("error : resource allocation failed \n");
  }

  maxBufferSz =0;
  free(src);
  free(outbuff);
  LZ4F_freeCompressionContext(ctx);
  return 0;
}


// @return : 1==error, 0==success
static int
decompress_internal(grpc_slice_buffer* input, grpc_slice_buffer* output,
                         LZ4F_dctx *dctx,
                         void *src, size_t srcCapacity,
                         void *dst, size_t dstCapacity)
{
    int firstChunk = 1;
    size_t ret = 1;

    assert(dctx != NULL);
    assert(src != NULL);
    assert(srcCapacity > 0);
    assert(dst != NULL);
    assert(dstCapacity > 0);

    for(size_t i = 0; i < input->count; i++) {
      void* inBufferPtr = GRPC_SLICE_START_PTR(input->slices[i]);
      size_t srcSize = GRPC_SLICE_LENGTH(input->slices[i]);

      ret = LZ4F_decompress(dctx, dst, &dstCapacity, inBufferPtr, &srcSize, NULL);
      if (LZ4F_isError(ret))
      {
        printf("Decompression error: %s\n", LZ4F_getErrorName(ret));
        return 1;
      }

      grpc_slice outbuf = GRPC_SLICE_MALLOC(dstCapacity);
      void* outBufferPtr = GRPC_SLICE_START_PTR(outbuf);
      memcpy(outBufferPtr, dst, dstCapacity);

      grpc_slice_buffer_add_indexed(output, outbuf);

    }

    return 0;
}

// @return : 1==error, 0==completed
static int decompress_slice_allocDst(grpc_slice_buffer* input, grpc_slice_buffer* output,
                         LZ4F_dctx *dctx,
                         void *src, size_t srcCapacity)
{
    assert(dctx != NULL);
    assert(src != NULL);
    assert(srcCapacity >= LZ4F_HEADER_SIZE_MAX); 

    // grpc_slice header = GRPC_SLICE_MALLOC(headerSize);
    void* headerBufferPtr = GRPC_SLICE_START_PTR( input->slices[0] );
    
    LZ4F_frameInfo_t info;
    size_t consumedSize = GRPC_SLICE_LENGTH( input->slices[0]);
    {
        size_t const fires = LZ4F_getFrameInfo(dctx, &info, headerBufferPtr, &consumedSize);
        if (LZ4F_isError(fires))
        {
            printf("LZ4F_getFrameInfo error: %s\n", LZ4F_getErrorName(fires));
            return 1;
        }
        if (consumedSize < GRPC_SLICE_LENGTH( input->slices[0]) ) {
          void* outBufferPtr = GRPC_SLICE_START_PTR( input->slices[0]);
          memmove(outBufferPtr, outBufferPtr + consumedSize, GRPC_SLICE_LENGTH( input->slices[0] ) - consumedSize);
          GRPC_SLICE_SET_LENGTH(input->slices[0], GRPC_SLICE_LENGTH( input->slices[0]) - consumedSize );
        }
    }


    size_t const dstCapacity = get_block_size(&info);
    void *const dst = malloc(dstCapacity);
    if (!dst)
    {
        perror("decompress_file(dst)");
        return 1;
    }

    int const decompressionResult = decompress_internal(
        input, output,
        dctx,
        src, srcCapacity,
        dst, dstCapacity);

    free(dst);
    return decompressionResult;
}

static int lz4_decompress(grpc_slice_buffer* input, grpc_slice_buffer* output) {
  
  size_t maxBufferSz = 0;
  for (size_t i = 0; i < input->count; i++) {
    if ( maxBufferSz < GRPC_SLICE_LENGTH( input->slices[i]) ) {
      maxBufferSz = GRPC_SLICE_LENGTH( input->slices[i]);
    }
  }

  size_t const outbufCapacity = LZ4F_compressBound(maxBufferSz, &kPrefs);

  void *const src = malloc(outbufCapacity);
  if (!src)
  {
      perror("decompress_file(src)");
      return 1;
  }


  LZ4F_dctx *dctx;
  {
    size_t const dctxStatus = LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);
    if (LZ4F_isError(dctxStatus))
    {
      printf("LZ4F_dctx creation error: %s\n", LZ4F_getErrorName(dctxStatus));
    }
  }

  int const result = !dctx ? 1: decompress_slice_allocDst(input, output, dctx, src, outbufCapacity);

  free(src);
  LZ4F_freeDecompressionContext(dctx);

  return 1;
}

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
