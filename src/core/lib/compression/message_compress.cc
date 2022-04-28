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
  for (int i = 0; i < input->count; i++) {
    auto sgSliceSZ = GRPC_SLICE_LENGTH(input->slices[i]);
    memcpy(srcBuffer + copiedSz,
           reinterpret_cast<char*> GRPC_SLICE_START_PTR(input->slices[i]),
           sgSliceSZ);
    copiedSz = copiedSz + sgSliceSZ;
  }

  // process lz4 frame header
  {
    size_t const outbufCapacity = LZ4F_compressBound(IN_CHUNK_SIZE, &kPrefs);
    char* headerBuff = reinterpret_cast<char*>(malloc(outbufCapacity));
    size_t const headerSize =
        LZ4F_compressBegin(ctx, headerBuff, outCapacity, &kPrefs);

    if (LZ4F_isError(headerSize)) {
      printf("Failed to start compression: error %u \n", (unsigned)headerSize);
      free(srcBuffer);
      free(headerBuff);
      return 1;
    }

    auto count_out = headerSize;
    printf("Buffer size is %u bytes, header size %u bytes \n",
           (unsigned)outCapacity, (unsigned)headerSize);

    grpc_slice outbuf = GRPC_SLICE_MALLOC(headerSize);
    // printf("malloc head buffers\n");
    char* headerBufferPtr =
        reinterpret_cast<char*> GRPC_SLICE_START_PTR(outbuf);
    // printf("copy head buffers\n");
    strncpy(headerBufferPtr, headerBuff, headerSize);
    grpc_slice_buffer_add_indexed(output, outbuf);
    // printf("free head buffers\n");
    free(headerBuff);
  }

  // printf("process bodys\n");

  size_t processedBytes = 0;
  void* outBuff = malloc(IN_CHUNK_SIZE);
  while (processedBytes < input->length) {
    size_t compressedSize;
    if (processedBytes + IN_CHUNK_SIZE > input->length) {
      compressedSize = LZ4F_compressUpdate(
          ctx, outBuff, outCapacity, srcBuffer + processedBytes,
          input->length - processedBytes, NULL);
      processedBytes = input->length;
    } else {
      compressedSize =
          LZ4F_compressUpdate(ctx, outBuff, outCapacity,
                              srcBuffer + processedBytes, IN_CHUNK_SIZE, NULL);

      processedBytes = processedBytes + IN_CHUNK_SIZE;
    }

    if (LZ4F_isError(compressedSize)) {
      free(srcBuffer);
      free(outBuff);
      printf("Compression failed: error %u \n", (unsigned)compressedSize);
      return -1;
    }

    // printf("malloc %u bytes \n", (unsigned)compressedSize);
    grpc_slice outbuf = GRPC_SLICE_MALLOC(compressedSize);
    char* outBufferPtr = reinterpret_cast<char*> GRPC_SLICE_START_PTR(outbuf);
    strncpy(outBufferPtr, srcBuffer + processedBytes, compressedSize);
    grpc_slice_buffer_add_indexed(output, outbuf);
  }

  {
    size_t const compressedSize =
        LZ4F_compressEnd(ctx, outBuff, outCapacity, NULL);

    if (LZ4F_isError(compressedSize)) {
      free(srcBuffer);
      free(outBuff);
      printf("Failed to end compression: error %u \n",
             (unsigned)compressedSize);
      return -1;
    }

    grpc_slice outbuf = GRPC_SLICE_MALLOC(compressedSize);
    char* outBufferPtr = reinterpret_cast<char*> GRPC_SLICE_START_PTR(outbuf);
    strncpy(outBufferPtr, srcBuffer + processedBytes, compressedSize);

    grpc_slice_buffer_add_indexed(output, outbuf);
  }

  free(outBuff);
  free(srcBuffer);
  return 0;
}

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

static int lz4_decompress(grpc_slice_buffer* input, grpc_slice_buffer* output) {
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
  for (int i = 0; i < input->count; i++) {
    auto sgSliceSZ = GRPC_SLICE_LENGTH(input->slices[i]);
    memcpy(srcBuffer + copiedSz,
           reinterpret_cast<char*> GRPC_SLICE_START_PTR(input->slices[i]),
           sgSliceSZ);
    copiedSz = copiedSz + sgSliceSZ;
  }

  LZ4F_dctx* dctx;
  {
    size_t const dctxStatus =
        LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);
    if (LZ4F_isError(dctxStatus)) {
      printf("LZ4F_dctx creation error: %s\n", LZ4F_getErrorName(dctxStatus));
      free(srcBuffer);
      return 1;
    }
  }

  if (!dctx) {
    LZ4F_freeDecompressionContext(dctx);
    free(srcBuffer);
    return 0;
  }

  size_t srcCapacity = IN_CHUNK_SIZE;

  // comsume header
  LZ4F_frameInfo_t info;
  {
    size_t const fires =
        LZ4F_getFrameInfo(dctx, &info, srcBuffer, &srcCapacity);
    if (LZ4F_isError(fires)) {
      printf("LZ4F_getFrameInfo error: %s\n", LZ4F_getErrorName(fires));
      LZ4F_freeDecompressionContext(dctx);
      free(srcBuffer);
      return 1;
    }
  }

  size_t const dstCapacity = get_block_size(&info);
  if (dstCapacity < 0) {
    perror("decompress_file(dst)");
    LZ4F_freeDecompressionContext(dctx);
    free(srcBuffer);
    return 1;
  }

  void* const dst = malloc(dstCapacity);
  if (!dst) {
    perror("decompress_file(dst)");
    LZ4F_freeDecompressionContext(dctx);
    free(srcBuffer);
    return 1;
  }

  size_t ret = 1;
  int firstChunk = 1;
  int comsumedSize = srcCapacity;
  while (comsumedSize < input->length ) {
    const void* srcStart = (const char*)srcBuffer + comsumedSize;    
    const void* const srcEnd = (const char*)srcBuffer + comsumedSize + srcCapacity; 
    while (srcStart < srcEnd && ret != 0) {
      size_t dstSize = dstCapacity;
      size_t srcSize = (const char*)srcEnd - (const char*)srcStart;
      ret = LZ4F_decompress(dctx, dst, &dstSize, srcStart, &srcSize, NULL);

      if (LZ4F_isError(ret)) {
        printf("Decompression error: %s\n", LZ4F_getErrorName(ret));
        return 1;
      }

      if (dstSize != 0){
        
        grpc_slice outbuf = GRPC_SLICE_MALLOC(dstSize);
        char* outBufferPtr = reinterpret_cast<char*> GRPC_SLICE_START_PTR(outbuf);
        strncpy(outBufferPtr, (char*) srcStart, dstSize);

        grpc_slice_buffer_add_indexed(output, outbuf);
      }
      
      srcStart = (const char*)srcStart + srcSize;
    }

    assert(srcStart <= srcEnd);

    if (srcStart < srcEnd) {
      free(srcBuffer);
      printf("Decompress: Trailing data left in file after frame\n");
      return 1;
    }

    comsumedSize = comsumedSize + srcCapacity;
  }

  LZ4F_freeDecompressionContext(dctx);
  free(srcBuffer);
  return 0;
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
