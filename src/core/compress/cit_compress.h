#ifndef CIT_COMPRESS_H
#define CIT_COMPRESS_H

#include <stddef.h>

/* Public API for Cit-LZ Compression */

/**
 * Calculates the maximum possible size of compressed data.
 */
size_t cit_compress_bound(size_t src_len);

/**
 * Compresses data from src to dst.
 * Returns 0 on success, -1 on failure.
 */
int cit_compress(const void *src, size_t src_len, void *dst, size_t *dst_len);

/**
 * Decompresses data from src to dst.
 * Returns 0 on success, -1 on failure.
 */
int cit_decompress(const void *src, size_t src_len, void *dst, size_t dst_len);

#endif // CIT_COMPRESS_H
