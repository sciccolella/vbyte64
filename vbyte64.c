#include "vbyte64.h"
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline uint8_t vb64_benc_noclz(uint64_t v,
                                      uint8_t *__restrict__ *data_pp) {
  uint8_t code = 9;
  if (v == 0) {
    code = 0;
  } else if (v < (1UL << 8)) {
    code = 1; // 1 byte
  } else if (v < (1UL << 16)) {
    code = 2; // 2 bytes
  } else if (v < (1UL << 24)) {
    code = 3; // 3 bytes
  } else if (v < (1UL << 32)) {
    code = 4; // 4 bytes
  } else if (v < (1UL << 40)) {
    code = 5; // 5 bytes
  } else if (v < (1UL << 48)) {
    code = 6; // 6 bytes
  } else if (v < (1UL << 56)) {
    code = 7; // 7 bytes
  } else {
    code = 8; // 8 bytes
  }
  memcpy(*data_pp, &v, code);
  *data_pp += code;
  return code;
}

static inline uint8_t vb64_benc(uint64_t v, uint8_t *__restrict__ *data_pp) {
  uint8_t code = v ? 8U - (__builtin_clzll(v | 1) >> 3) : 0;
  memcpy(*data_pp, &v, code);
  *data_pp += code;
  return code;
}

size_t vb64d_encode_size(const uint64_t *v, size_t n) {
  size_t nbytes = 0, i;
  uint64_t vo_ = v[0], v_ = vo_;
  nbytes = vo_ ? 8U - (__builtin_clzll(vo_ | 1) >> 3) : 0;
  for (i = 1; i < n; ++i) {
    v_ = v[i];
    nbytes += v_ - vo_ ? 8U - (__builtin_clzll((v_ - vo_) | 1) >> 3) : 0;
    vo_ = v_;
  }
  return nbytes;
}

size_t vb64_encode_size(const uint64_t *v, size_t n) {
  size_t nbytes = 0, i;
  // uint64_t vo_ = v[0], v_ = vo_;
  // nbytes = vo_ ? 8U - (__builtin_clzll(vo_ | 1) >> 3) : 0;
  for (i = 0; i < n; ++i) {
    nbytes += v[i] ? 8U - (__builtin_clzll(v[i] | 1) >> 3) : 0;
  }
  return nbytes;
}

size_t vb64d_encode_size_noclz(const uint64_t *v, size_t n) {
  size_t nbytes = 0, i;
  uint64_t vo_ = v[0], v_ = vo_;
  nbytes += (v_ > 0) + (v_ > 0x00000000000000FF) + (v_ > 0x000000000000FFFF) +
            (v_ > 0x0000000000FFFFFF) + (v_ > 0x00000000FFFFFFFF) +
            (v_ > 0x000000FFFFFFFFFF) + (v_ > 0x0000FFFFFFFFFFFF) +
            (v_ > 0x00FFFFFFFFFFFFFF);
  for (i = 1; i < n; ++i) {
    v_ = v[i];
    nbytes +=
        ((v_ - vo_) > 0) + ((v_ - vo_) > 0x00000000000000FF) +
        ((v_ - vo_) > 0x000000000000FFFF) + ((v_ - vo_) > 0x0000000000FFFFFF) +
        ((v_ - vo_) > 0x00000000FFFFFFFF) + ((v_ - vo_) > 0x000000FFFFFFFFFF) +
        ((v_ - vo_) > 0x0000FFFFFFFFFFFF) + ((v_ - vo_) > 0x00FFFFFFFFFFFFFF);
    vo_ = v_;
  }
  return nbytes;
}

size_t vb64_encode_size_noclz(const uint64_t *v, size_t n) {
  size_t nbytes = 0, i;
  uint64_t v_;
  for (i = 0; i < n; ++i) {
    v_ = v[i];
    nbytes += (v_ > 0) + (v_ > 0x00000000000000FF) + (v_ > 0x000000000000FFFF) +
              (v_ > 0x0000000000FFFFFF) + (v_ > 0x00000000FFFFFFFF) +
              (v_ > 0x000000FFFFFFFFFF) + (v_ > 0x0000FFFFFFFFFFFF) +
              (v_ > 0x00FFFFFFFFFFFFFF);
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
size_t vb64d_compressed_size(const uint64_t *v, size_t n) {
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

/*
 * Calculate the exact size required to compress array `v` of size `n`.
 * Note this only calculate the size to encode the data, not the length of the
 * array. Depending on the flag `VBYTE64_NO_CLZ` it will call the function using
 * `CLZ` (COUNT LEADING ZEROES) or the one using comparison.
 */
size_t vb64_compressed_size(const uint64_t *v, size_t n) {
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
  size_t data_size = vb64_encode_size_noclz(v, n);
#else
  size_t data_size = vb64_encode_size(v, n);
#endif /* ifdef VBYTE64_NO_CLZ */
  return key_size + data_size + VBYTE64_PADDING;
}

static uint8_t *vb64_encode_delta(uint8_t *key_p, uint8_t *data_p,
                                  const uint64_t *v, size_t n) {
  uint8_t shift_ = 0, ckey = 0, code = 0;
  uint64_t ov = v[0], cv;
  // first one must be encoded fully
#ifdef VBYTE64_NO_CLZ
  code = vb64_benc_noclz(v[0], &data_p);
#else
  code = vb64_benc(v[0], &data_p);
#endif /* ifdef VBYTE64_NO_CLZ */

  ckey |= code << shift_;
  shift_ += 4;

  for (size_t i = 1; i < n; i++) {
    if (shift_ == 8) {
      shift_ = 0;
      *key_p++ = ckey;
      ckey = 0;
    }
    cv = v[i];
#ifdef VBYTE64_NO_CLZ
    code = vb64_benc_noclz(cv - ov, &data_p);
#else
    code = vb64_benc(cv - ov, &data_p);
#endif /* ifdef VBYTE64_NO_CLZ */
    ckey |= code << shift_;
    shift_ += 4;
    ov = cv;
  }
  *key_p++ = ckey;
  // pointer to first unused
  return data_p;
}

static uint8_t *vb64_encode(uint8_t *key_p, uint8_t *data_p, const uint64_t *v,
                            size_t n) {
  uint8_t shift_ = 0, ckey = 0, code = 0;
  for (size_t i = 0; i < n; i++) {
    if (shift_ == 8) {
      shift_ = 0;
      *key_p++ = ckey;
      ckey = 0;
    }
#ifdef VBYTE64_NO_CLZ
    code = vb64_benc_noclz(v[i], &data_p);
#else
    code = vb64_benc(v[i], &data_p);
#endif /* ifdef VBYTE64_NO_CLZ */
    ckey |= code << shift_;
    shift_ += 4;
  }
  *key_p++ = ckey;

  // pointer to first unused
  return data_p;
}

uint8_t *vb64_compress_delta(uint64_t *v, size_t n, size_t *clen) {
  size_t key_size = sizeof(uint8_t) * ((n + 1) / 2);
  size_t data_size = vb64d_encode_size(v, n);
  size_t compress_size = key_size + data_size + VBYTE64_PADDING;

  uint8_t *cdata = (uint8_t *)malloc(compress_size);
  if (!cdata)
    return NULL;
  uint8_t *key_p = cdata;
  uint8_t *data_p = key_p + key_size;

  uint8_t *data_p_end = vb64_encode_delta(key_p, data_p, v, n);

  if (clen)
    *clen = data_p_end - cdata;
  return cdata;
}

uint8_t *vb64_compress(uint64_t *v, size_t n, size_t *clen) {
  size_t key_size = sizeof(uint8_t) * ((n + 1) / 2);
  size_t data_size = vb64_encode_size(v, n);
  size_t compress_size = key_size + data_size + VBYTE64_PADDING;

  uint8_t *cdata = (uint8_t *)malloc(compress_size);
  if (!cdata)
    return NULL;
  uint8_t *key_p = cdata;
  uint8_t *data_p = key_p + key_size;

  uint8_t *data_p_end = vb64_encode(key_p, data_p, v, n);

  if (clen)
    *clen = data_p_end - cdata;
  return cdata;
}

uint8_t *vb64_compress_delta_wl(uint64_t *v, size_t n, size_t *clen) {
  size_t key_size = sizeof(size_t) + sizeof(uint8_t) * ((n + 1) / 2);
  size_t data_size = vb64d_encode_size(v, n);
  size_t compress_size = key_size + data_size + VBYTE64_PADDING;

  uint8_t *cdata = (uint8_t *)malloc(compress_size);
  if (!cdata)
    return NULL;
  uint8_t *key_p = cdata;
  // copy size to the first bytes
  memcpy(key_p, &n, sizeof(size_t));

  uint8_t *data_p = key_p + key_size;
  key_p += sizeof(size_t);

  uint8_t *data_p_end = vb64_encode_delta(key_p, data_p, v, n);
  if (clen)
    *clen = data_p_end - cdata;
  return cdata;
}

uint8_t *vb64_compress_wl(uint64_t *v, size_t n, size_t *clen) {
  size_t key_size = sizeof(size_t) + sizeof(uint8_t) * ((n + 1) / 2);
  size_t data_size = vb64_encode_size(v, n);
  size_t compress_size = key_size + data_size + VBYTE64_PADDING;

  uint8_t *cdata = (uint8_t *)malloc(compress_size);
  if (!cdata)
    return NULL;
  uint8_t *key_p = cdata;
  // copy size to the first bytes
  memcpy(key_p, &n, sizeof(size_t));

  uint8_t *data_p = key_p + key_size;
  key_p += sizeof(size_t);

  uint8_t *data_p_end = vb64_encode(key_p, data_p, v, n);

  if (clen)
    *clen = data_p_end - cdata;
  return cdata;
}

static inline uint64_t vb64_bdec(const uint8_t **data_pp, uint8_t code) {
  // uint64_t val = 0;
  // const uint8_t *data_p = *data_pp;
  // memcpy(&val, data_p, code);
  // data_p += code;
  // *data_pp = data_p;
  // return val;

  uint64_t val = 0;
  memcpy(&val, *data_pp, code);
  *data_pp += code;
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

static const uint8_t *vb64_decode(const uint8_t *key_p, const uint8_t *data_p,
                                  uint64_t *o, size_t n) {
  if (n == 0)
    return data_p;

  uint8_t shift_ = 0, key = *key_p++;

  for (size_t i = 0; i < n; ++i) {
    if (shift_ == 8) {
      shift_ = 0;
      key = *key_p++;
    }

    *o++ = vb64_bdec(&data_p, (key >> shift_) & 0xF);
    shift_ += 4;
  }

  return data_p;
}

void vb64_decompress_delta(uint8_t *in, uint64_t *out, size_t n) {
  size_t key_size = sizeof(uint8_t) * ((n + 1) / 2);
  uint8_t *key_p = in;
  uint8_t *data_p = key_p + key_size;

  vb64_decode_delta(key_p, data_p, out, n);
}

void vb64_decompress(uint8_t *in, uint64_t *out, size_t n) {
  size_t key_size = sizeof(uint8_t) * ((n + 1) / 2);
  uint8_t *key_p = in;
  uint8_t *data_p = key_p + key_size;

  vb64_decode(key_p, data_p, out, n);
}

uint64_t *vb64_decompress_delta_wl(uint8_t *in, size_t *n) {
  memcpy(n, in, sizeof(size_t));
  uint64_t *out = malloc(sizeof(out[0]) * *n);
  if (!out)
    return NULL;
  size_t key_size = sizeof(uint8_t) * ((*n + 1) / 2);
  uint8_t *key_p = in + sizeof(size_t);
  uint8_t *data_p = key_p + key_size;

  vb64_decode_delta(key_p, data_p, out, *n);
  return out;
}

uint64_t *vb64_decompress_wl(uint8_t *in, size_t *n) {
  memcpy(n, in, sizeof(size_t));
  uint64_t *out = malloc(sizeof(out[0]) * *n);
  if (!out)
    return NULL;
  size_t key_size = sizeof(uint8_t) * ((*n + 1) / 2);
  uint8_t *key_p = in + sizeof(size_t);
  uint8_t *data_p = key_p + key_size;

  vb64_decode(key_p, data_p, out, *n);
  return out;
}

// Compression using files directly

enum vb64f_state {
  vb64f_ok = 0,
  vb64f_rerr,
  vb64f_werr,
};

static inline uint8_t vb64f_benc_noclz(uint64_t v, FILE *cf_data,
                                       jmp_buf jmp_target) {
  uint8_t code = 9;
  if (v == 0) {
    return 0;
  } else if (v < (1UL << 8)) {
    code = 1; // 1 byte
  } else if (v < (1UL << 16)) {
    code = 2; // 2 bytes
  } else if (v < (1UL << 24)) {
    code = 3; // 3 bytes
  } else if (v < (1UL << 32)) {
    code = 4; // 4 bytes
  } else if (v < (1UL << 40)) {
    code = 5; // 5 bytes
  } else if (v < (1UL << 48)) {
    code = 6; // 6 bytes
  } else if (v < (1UL << 56)) {
    code = 7; // 7 bytes
  } else {
    code = 8; // 8 bytes
  }
  if (fwrite(&v, sizeof(uint8_t) * code, 1, cf_data) != 1) {
    longjmp(jmp_target, vb64f_werr);
  }
  return code;
}

static inline uint8_t vb64f_benc(uint64_t v, FILE *cf_data,
                                 jmp_buf jmp_target) {
  if (!v)
    return 0;
  uint8_t code = 8U - (__builtin_clzll(v | 1) >> 3);
  if (fwrite(&v, sizeof(uint8_t) * code, 1, cf_data) != 1) {
    longjmp(jmp_target, vb64f_werr);
  }
  return code;
}

size_t vb64f_encode_delta(FILE *cf_key, FILE *cf_data, const uint64_t *v,
                          size_t n, jmp_buf jmp_target) {
  size_t nbytes = 0;
  uint8_t shift_ = 0, ckey = 0, code = 0;
  uint64_t ov = v[0], cv;
  // first one must be encoded fully
#ifdef VBYTE64_NO_CLZ
  code = vb64f_benc_noclz(v[0], cf_data, jmp_target);
#else
  code = vb64f_benc(v[0], cf_data, jmp_target);
#endif /* ifdef VBYTE64_NO_CLZ */
  nbytes += code;

  ckey |= code << shift_;
  shift_ += 4;

  for (size_t i = 1; i < n; i++) {
    if (shift_ == 8) {
      shift_ = 0;
      if (fwrite(&ckey, sizeof(uint8_t), 1, cf_key) != 1) {
        longjmp(jmp_target, vb64f_werr);
      }
      ckey = 0;
      ++nbytes;
    }
    cv = v[i];
#ifdef VBYTE64_NO_CLZ
    code = vb64f_benc_noclz(cv - ov, cf_data, jmp_target);
#else
    code = vb64f_benc(cv - ov, cf_data, jmp_target);
#endif /* ifdef VBYTE64_NO_CLZ */
    nbytes += code;
    ckey |= code << shift_;
    shift_ += 4;
    ov = cv;
  }
  if (fwrite(&ckey, sizeof(uint8_t), 1, cf_key) != 1) {
    longjmp(jmp_target, vb64f_werr);
  }
  return ++nbytes;
}

size_t vb64f_compress_delta(uint64_t *v, size_t n, const char *fpath) {
  size_t key_size = sizeof(size_t) + sizeof(uint8_t) * ((n + 1) / 2),
         nbytes = 0;

  FILE *cf_key = fopen(fpath, "wb");
  FILE *cf_data = fopen(fpath, "r+b");

  if (!cf_key || !cf_data)
    return 0;

  jmp_buf jmp_target;

  switch (setjmp(jmp_target)) {
  case 0:
    break;
  case vb64f_werr:
    fprintf(stderr, "[%s] ERROR: fwrite failed, exit.\n", __func__);
    nbytes = 0;
    goto clean;
  default:
    fprintf(stderr, "[%s] An unknown error have occurred, exit.\n", __func__);
    exit(EXIT_FAILURE);
  }
  // copy size to the first bytes
  if (fwrite(&n, sizeof(size_t), 1, cf_key) != 1) {
    longjmp(jmp_target, vb64f_werr);
  }

  fseek(cf_data, key_size, SEEK_SET);
  nbytes =
      vb64f_encode_delta(cf_key, cf_data, v, n, jmp_target) + sizeof(size_t);

clean:
  fclose(cf_data);
  fclose(cf_key);
  return nbytes;
}

// DECODE
static inline uint64_t vb64f_bdec(FILE *cf_data, uint8_t code,
                                  jmp_buf jmp_target) {
  if (code == 0)
    return 0;

  uint64_t val = 0;
  if (fread(&val, sizeof(uint8_t), code, cf_data) != code) {
    longjmp(jmp_target, vb64f_rerr);
  }
  return val;
}

void vb64f_decode_delta(FILE *cf_keys, FILE *cf_data, uint64_t *o, size_t n,
                        jmp_buf jmp_target) {
  // first value if fully encoded
  uint8_t key = 0;
  if (fread(&key, sizeof(uint8_t), 1, cf_keys) != 1) {
    longjmp(jmp_target, vb64f_rerr);
  }
  uint64_t prev = vb64f_bdec(cf_data, key & 0xF, jmp_target), val = 0;

  *o++ = prev;
  uint8_t shift_ = 4;

  for (size_t i = 1; i < n; ++i) {
    if (shift_ == 8) {
      shift_ = 0;
      if (fread(&key, sizeof(uint8_t), 1, cf_keys) != 1) {
        longjmp(jmp_target, vb64f_rerr);
      }
    }
    val = vb64f_bdec(cf_data, (key >> shift_) & 0xF, jmp_target);
    val += prev;
    *o++ = val;
    prev = val;
    shift_ += 4;
  }
}

uint64_t *vb64f_decompress_delta(const char *fpath, size_t *n) {
  FILE *cf_keys = fopen(fpath, "rb");
  FILE *cf_data = fopen(fpath, "rb");
  uint64_t *out;

  if (!cf_keys || !cf_data)
    return NULL;

  jmp_buf jmp_target;

  switch (setjmp(jmp_target)) {
  case 0:
    break;
  case vb64f_rerr:
    fprintf(stderr, "[%s] ERROR: fread failed, exit.\n", __func__);
    out = NULL;
    goto clean;
  default:
    fprintf(stderr, "[%s] An unknown error have occurred, exit.\n", __func__);
    exit(EXIT_FAILURE);
  }

  if (fread(n, sizeof(size_t), 1, cf_keys) != 1) {
    longjmp(jmp_target, vb64f_rerr);
  }

  size_t key_size = sizeof(size_t) + sizeof(uint8_t) * ((*n + 1) / 2);
  size_t data_offset = key_size;
  fseek(cf_data, data_offset, SEEK_SET);
  out = malloc(sizeof(out[0]) * *n);

  if (!out)
    return NULL;

  vb64f_decode_delta(cf_keys, cf_data, out, *n, jmp_target);

clean:
  fclose(cf_data);
  fclose(cf_keys);
  return out;
}
