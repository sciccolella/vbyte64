#include "vbyte64.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static inline uint8_t vb64_benc_noclz(uint64_t v,
                                      uint8_t *__restrict__ *data_pp) {
  uint8_t code = 9;
  if (v < (1UL << 8)) {
    code = 0; // 1 byte
  } else if (v < (1UL << 16)) {
    code = 1; // 2 bytes
  } else if (v < (1UL << 24)) {
    code = 2; // 3 bytes
  } else if (v < (1UL << 32)) {
    code = 3; // 4 bytes
  } else if (v < (1UL << 40)) {
    code = 4; // 5 bytes
  } else if (v < (1UL << 48)) {
    code = 5; // 6 bytes
  } else if (v < (1UL << 56)) {
    code = 6; // 7 bytes
  } else {
    code = 7; // 8 bytes
  }
  memcpy(*data_pp, &v, code + 1);
  *data_pp += code + 1;
  return code;
}

static inline uint8_t vb64_benc(uint64_t v, uint8_t *__restrict__ *data_pp) {
  uint8_t code = 7U - (__builtin_clzll(v | 1) >> 3);
  memcpy(*data_pp, &v, code + 1);
  *data_pp += code + 1;
  return code;
}

size_t vb64d_encode_size(const uint64_t *v, size_t n) {
  size_t nbytes = 0, i;
  uint64_t _vo = v[0], _v = _vo;
  nbytes = 8U - (__builtin_clzll(_vo | 1) >> 3);
  for (i = 1; i < n; ++i) {
    _v = v[i];
    nbytes += 8U - (__builtin_clzll((_v - _vo) | 1) >> 3);
    _vo = _v;
  }
  return nbytes;
}

size_t vb64d_encode_size_noclz(const uint64_t *v, size_t n) {
  size_t nbytes = 0, i;
  uint64_t _vo = v[0], _v = _vo;
  nbytes += 1 + (_v > 0x00000000000000FF) + (_v > 0x000000000000FFFF) +
            (_v > 0x0000000000FFFFFF) + (_v > 0x00000000FFFFFFFF) +
            (_v > 0x000000FFFFFFFFFF) + (_v > 0x0000FFFFFFFFFFFF) +
            (_v > 0x00FFFFFFFFFFFFFF);
  for (i = 1; i < n; ++i) {
    _v = v[i];
    nbytes +=
        1 + ((_v - _vo) > 0x00000000000000FF) +
        ((_v - _vo) > 0x000000000000FFFF) + ((_v - _vo) > 0x0000000000FFFFFF) +
        ((_v - _vo) > 0x00000000FFFFFFFF) + ((_v - _vo) > 0x000000FFFFFFFFFF) +
        ((_v - _vo) > 0x0000FFFFFFFFFFFF) + ((_v - _vo) > 0x00FFFFFFFFFFFFFF);
    _vo = _v;
  }
  return nbytes;
}

/*
 * Calculate the exact size required to compress array `v` of size `n`
 * using delta variable byte encoding.
 * Note this only calculate the size to encode the data, not the length of the
 * array. Depending on the flag `VBYTE64_NO_CLZ` it will call the function using
 * `CLZ` (COUNT LEADING ZEROES) or the one using comparison.
 */
static size_t vb64d_compressed_size(const uint64_t *v, size_t n) {
  // 8 bits will encode the code for 2 values
  // because of values being uint64, we need 4 bits for key
  // therefore the number of keys is the LEN(values) / 2
  // but wee need to round up in case LEN is odd => len + 1
  //
  // NOTE: in the original case of values as uint32
  // 8 bits can encode 4 keys => the original len
  // of the keys is (len+3)/4
  size_t key_size = sizeof(uint8_t) * ((n + 1) / 2);
#ifdef VBYTE64_NO_CLZ
  size_t data_size = vb64d_encode_size_noclz(v, n);
#else
  size_t data_size = vb64d_encode_size(v, n);
#endif /* ifdef VBYTE64_NO_CLZ */
  return key_size + data_size + VBYTE64_PADDING;
}

static uint8_t *vb64_encode_delta(uint8_t *key_p, uint8_t *data_p,
                                  const uint64_t *v, size_t n) {
  uint8_t shift_ = 0, ckey = 0, code = 0;
  uint64_t ov = v[0], cv;
  // first one must be encoded fully
  code = vb64_benc(v[0], &data_p);
  ckey |= code << shift_;
  shift_ += 4;

  for (size_t i = 1; i < n; i++) {
    if (shift_ == 8) {
      shift_ = 0;
      *key_p++ = ckey;
      ckey = 0;
    }
    cv = v[i];
    code = vb64_benc(cv - ov, &data_p);
    ckey |= code << shift_;
    shift_ += 4;
    ov = cv;
  }
  *key_p++ = ckey;
  return data_p;
}

/*
 * Compress data in vector `v` of size `n` using variable byte encoding.
 * Returns a pointer of `uint8_t` containing the compressed data.
 * Return `NULL` if allocation of the uncompressed array fails.
 */
uint8_t *vb64_compress_delta(uint64_t *v, size_t n) {
  size_t key_size = sizeof(uint8_t) * ((n + 1) / 2);
  size_t data_size = vb64d_encode_size(v, n);
  size_t compress_size = key_size + data_size + VBYTE64_PADDING;

  uint8_t *cdata = (uint8_t *)malloc(compress_size);
  if (!cdata)
    return NULL;
  uint8_t *key_p = cdata;
  uint8_t *data_p = key_p + key_size;

  vb64_encode_delta(key_p, data_p, v, n);
  return cdata;
}

/*
 * Compress data in vector `v` of size `n` using variable byte encoding.
 * Returns a pointer of `uint8_t` containing the compressed data.
 * This version utilizes the first `sizeof(size_t)` bytes of the compressed
 * data to store the length of the array.
 * Return `NULL` if allocation of the uncompressed array fails.
 */
uint8_t *vb64_compress_delta_wl(uint64_t *v, size_t n) {
  size_t key_size = sizeof(size_t) + sizeof(uint8_t) * ((n + 1) / 2);
  size_t data_size = vb64d_encode_size(v, n);
  size_t compress_size = key_size + data_size + VBYTE64_PADDING;

  uint8_t *cdata = (uint8_t *)malloc(compress_size);
  if (!cdata)
    return NULL;
  uint8_t *key_p = cdata;
  // copy size to the first bytes
  memcpy(key_p, &n, sizeof(size_t));
  key_p += sizeof(size_t);

  uint8_t *data_p = key_p + key_size;

  vb64_encode_delta(key_p, data_p, v, n);
  return cdata;
}

static inline uint64_t vb64_bdec(const uint8_t **data_pp, uint8_t code) {
  uint64_t val = 0;
  const uint8_t *data_p = *data_pp;
  memcpy(&val, data_p, code + 1);
  data_p += code + 1;
  *data_pp = data_p;
  return val;
}

static const uint8_t *vb64_decode_delta(const uint8_t *key_p,
                                        const uint8_t *data_p, uint64_t *o,
                                        size_t n) {
  if (n == 0)
    return data_p;

  // first value if fully encoded
  uint8_t key = *key_p++;
  uint64_t prev = vb64_bdec(&data_p, key & 0xF), val = 0;
  *o++ = prev;
  uint8_t shift_ = 4;

  for (size_t i = 1; i < n; ++i) {
    if (shift_ == 8) {
      shift_ = 0;
      key = *key_p++;
    }

    val = vb64_bdec(&data_p, (key >> shift_) & 0xF);
    val += prev;
    *o++ = val;
    prev = val;
    shift_ += 4;
  }

  return data_p;
}

/*
 * Decompress data in vector `in` of size `n` using variable byte decoding.
 * This version requires to know the length of the compressed array and the
 * `out` array should be already allocated.
 */
static void vb64_decompress_delta(uint8_t *in, uint64_t *out, size_t n) {
  size_t key_size = sizeof(uint8_t) * ((n + 1) / 2);
  uint8_t *key_p = in;
  uint8_t *data_p = key_p + key_size;

  vb64_decode_delta(key_p, data_p, out, n);
}

/*
 * Decompress data in vector `in` using variable byte decoding of unknown size.
 * Provide a valid pointer to a variable `n` to store the retrieved lenght of
 * the array. Returns a pointer of `uint16_t` containing the uncompressed data.
 * This version utilizes the first `sizeof(size_t)` bytes of the compressed
 * data to store the length of the array.
 * Return `NULL` if allocation of the uncompressed array fails.
 */
static uint64_t *vb64_decompress_delta_wl(uint8_t *in, size_t *n) {
  memcpy(n, in, sizeof(size_t));
  uint64_t *out = malloc(sizeof(out[0]) * *n);
  if (!out)
    return NULL;
  size_t key_size = sizeof(size_t) + sizeof(uint8_t) * ((*n + 1) / 2);
  uint8_t *key_p = in + sizeof(size_t);
  uint8_t *data_p = key_p + key_size;

  vb64_decode_delta(key_p, data_p, out, *n);
  return out;
}

void sanity_encode_check() {
  uint64_t data[] = {
      0,
      13,
      16,
      17,
      20,
      241UL,                 // 11110000 (1 byte)
      65282UL,               // (2 byte)
      16776963UL,            // 3
      4294967044UL,          // 4
      1099511562245UL,       // 5
      281474959933446UL,     // 6
      72057589743656967UL,   // 7
      18446742974376182038UL // 8
  };
  size_t n = sizeof data / sizeof data[0];
  printf("len = %zu\n", n);

  uint8_t *compressed = vb64_compress_delta_wl(data, n);
  size_t clen = 0;
  uint64_t *decompressed = vb64_decompress_delta_wl(compressed, &clen);

  for (size_t i = 0; i < n; i++) {
    printf("data[i] = %lu\n", data[i]);
    printf("deco[i] = %lu\n", decompressed[i]);
  }

  free(compressed);
  free(decompressed);
}

void benchmark_decode(size_t n) {
  clock_t t0 = clock();

  printf("n = %zu\n", n);
  uint64_t *au64 = malloc(n * sizeof au64[0]);

  for (size_t i = 0; i < n; i++) {
    au64[i] |= rand();
    au64[i] = (au64[i] << 32) | rand();
  }
  printf("Generated\n");

  uint8_t *compressed = vb64_compress_delta_wl(au64, n);
  printf("Compressed\n");
  t0 = clock();
  size_t clen = 0;
  uint64_t *decompressed = vb64_decompress_delta_wl(compressed, &clen);
  fprintf(stderr, "[decode] decoded: %5ld - dumped : %5ld\n",
          (clock() - t0) / CLOCKS_PER_SEC, 0UL);
  printf("[decoded] clen = %zu\n", clen);

  for (size_t i = 0; i < n; i++) {
    if (au64[i] != decompressed[i])
      printf("ERROR\n");
  }

  free(au64);
  free(compressed);
  free(decompressed);
}

int main(int argc, char *argv[]) {
  // benchmark_decode(5e5);
  // sanity_encode_check();
  return EXIT_SUCCESS;
}
