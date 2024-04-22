#ifndef VBYTE64_H
#define VBYTE64_H

// #define VBYTE64_NO_CLZ
#define VBYTE64_PADDING 64

#ifdef __cplusplus
#include <cstdint>
#include <cstdlib>
extern "C" {
#else
#include <stdint.h>
#include <stdlib.h>
#endif // __cplusplus

/*
 * Calculate the exact size required to compress array `v` of size `n`
 * using delta variable byte encoding.
 * Note this only calculate the size to encode the data, not the length of the
 * array. Depending on the flag `VBYTE64_NO_CLZ` it will call the function using
 * `CLZ` (COUNT LEADING ZEROES) or the one using comparison.
 */
static size_t vb64d_compressed_size(const uint64_t *v, size_t n);

/*
 * Compress data in vector `v` of size `n` using variable byte encoding.
 * Returns a pointer of `uint8_t` containing the compressed data.
 * Return `NULL` if allocation of the uncompressed array fails.
 */
uint8_t *vb64_compress_delta(uint64_t *v, size_t n);

/*
 * Compress data in vector `v` of size `n` using variable byte encoding.
 * Returns a pointer of `uint8_t` containing the compressed data.
 * This version utilizes the first `sizeof(size_t)` bytes of the compressed
 * data to store the length of the array.
 * Return `NULL` if allocation of the uncompressed array fails.
 */
uint8_t *vb64_compress_delta_wl(uint64_t *v, size_t n);

/*
 * Decompress data in vector `in` of size `n` using variable byte decoding.
 * This version requires to know the length of the compressed array and the
 * `out` array should be already allocated.
 */
static void vb64_decompress_delta(uint8_t *in, uint64_t *out, size_t n);

/*
 * Decompress data in vector `in` using variable byte decoding of unknown size.
 * Provide a valid pointer to a variable `n` to store the retrieved lenght of
 * the array. Returns a pointer of `uint16_t` containing the uncompressed data.
 * This version utilizes the first `sizeof(size_t)` bytes of the compressed
 * data to store the length of the array.
 * Return `NULL` if allocation of the uncompressed array fails.
 */
static uint64_t *vb64_decompress_delta_wl(uint8_t *in, size_t *n);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !VBYTE64_H
