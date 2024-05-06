#include "vbyte64.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

void sanity_check() {
  uint64_t data[] = {
      0,
      13,
      16,
      17,
      20,
      20,
      241UL,                 // 11110000 (1 byte)
      65282UL,               // (2 byte)
      16776963UL,            // 3
      16776963UL,            // 3
      4294967044UL,          // 4
      1099511562245UL,       // 5
      281474959933446UL,     // 6
      72057589743656967UL,   // 7
      18446742974376182038UL // 8
  };
  size_t n = sizeof data / sizeof data[0];
  printf("len = %zu\n", n);

  uint8_t *compressed = vb64_compress(data, n, NULL);
  uint64_t *decompressed = malloc(sizeof *decompressed * n);
  vb64_decompress(compressed, decompressed, n);

  for (size_t i = 0; i < n; i++) {
    printf("data[%2zu] = %lu\n", i, data[i]);
    printf("deco[%2zu] = %lu\n", i, decompressed[i]);
  }

  free(compressed);
  free(decompressed);
}
void sanity_check_wl() {
  uint64_t data[] = {
      0,
      13,
      16,
      17,
      20,
      20,
      241UL,                 // 11110000 (1 byte)
      65282UL,               // (2 byte)
      16776963UL,            // 3
      16776963UL,            // 3
      4294967044UL,          // 4
      1099511562245UL,       // 5
      281474959933446UL,     // 6
      72057589743656967UL,   // 7
      18446742974376182038UL // 8
  };
  size_t n = sizeof data / sizeof data[0];
  printf("len = %zu\n", n);

  size_t compressed_len = 0;
  uint8_t *compressed = vb64_compress_delta_wl(data, n, &compressed_len);

  FILE *fcomp = fopen("tcomp.bin", "w");
  fwrite(compressed, sizeof(uint8_t), compressed_len, fcomp);
  fclose(fcomp);

  size_t clen = 0;
  uint64_t *decompressed = vb64_decompress_delta_wl(compressed, &clen);
  if (!decompressed)
    exit(EXIT_FAILURE);

  for (size_t i = 0; i < clen; i++) {
    printf("data[%2zu] = %lu\n", i, data[i]);
    printf("deco[%2zu] = %lu\n", i, decompressed[i]);
  }

  free(compressed);
  free(decompressed);
}
void sanity_check_file() {
  uint64_t data[] = {
      0,
      13,
      16,
      17,
      20,
      20,
      241UL,                 // 11110000 (1 byte)
      65282UL,               // (2 byte)
      16776963UL,            // 3
      16776963UL,            // 3
      4294967044UL,          // 4
      1099511562245UL,       // 5
      281474959933446UL,     // 6
      72057589743656967UL,   // 7
      18446742974376182038UL // 8
  };
  size_t n = sizeof data / sizeof data[0];
  printf("len = %zu\n", n);

  // size_t compressed_len = 0;
  // uint8_t *compressed = vb64_compress_delta_wl(data, n, &compressed_len);
  size_t compressed_len = vb64f_compress_delta(data, n, "fcomp.bin");
  if (!compressed_len)
    exit(EXIT_FAILURE);

  size_t clen = 0;
  uint64_t *decompressed = vb64f_decompress_delta("fcomp.bin", &clen);
  if (!decompressed)
    exit(EXIT_FAILURE);

  for (size_t i = 0; i < clen; i++) {
    printf("data[%2zu] = %lu\n", i, data[i]);
    printf("deco[%2zu] = %lu\n", i, decompressed[i]);
  }

  free(decompressed);
}

void test_encdec_delta(size_t n) {
  fprintf(stderr, "\n[%s]\n", __func__);
  fprintf(stderr, "[gen ] n = %zu\n", n);

  clock_t t0 = clock();
  uint64_t *au64 = malloc(n * sizeof au64[0]);
  for (size_t i = 0; i < n; i++) {
    au64[i] |= rand();
    au64[i] = (au64[i] << 32) | rand();
  }
  fprintf(stderr, "[gen ] generated: %5ld s\n",
          (clock() - t0) / CLOCKS_PER_SEC);

  t0 = clock();
  size_t compressed_len = 0;
  uint8_t *compressed = vb64_compress_delta_wl(au64, n, &compressed_len);
  fprintf(stderr, "[encode] encoded: %5ld s\n",
          (clock() - t0) / CLOCKS_PER_SEC);

  t0 = clock();
  size_t clen = 0;
  uint64_t *decompressed = vb64_decompress_delta_wl(compressed, &clen);
  fprintf(stderr, "[decode] decoded: %5ld s\n",
          (clock() - t0) / CLOCKS_PER_SEC);

  fprintf(stderr, "[decode] n = %zu\n", clen);

  size_t errors = 0;
  for (size_t i = 0; i < n; i++) {
    errors += (au64[i] != decompressed[i]);
  }
  fprintf(stderr, "[decode] errors = %zu\n", errors);

  free(au64);
  free(compressed);
  free(decompressed);
}

void test_encdec(size_t n) {
  fprintf(stderr, "\n[%s]\n", __func__);
  fprintf(stderr, "[gen ] n = %zu\n", n);

  clock_t t0 = clock();
  uint64_t *au64 = malloc(n * sizeof au64[0]);
  for (size_t i = 0; i < n; i++) {
    au64[i] |= rand();
    au64[i] = (au64[i] << 32) | rand();
  }
  fprintf(stderr, "[gen ] generated: %5ld s\n",
          (clock() - t0) / CLOCKS_PER_SEC);

  t0 = clock();
  size_t compressed_len = 0;
  uint8_t *compressed = vb64_compress_wl(au64, n, &compressed_len);
  fprintf(stderr, "[encode] encoded: %5ld s\n",
          (clock() - t0) / CLOCKS_PER_SEC);

  t0 = clock();
  size_t clen = 0;
  uint64_t *decompressed = vb64_decompress_wl(compressed, &clen);
  fprintf(stderr, "[decode] decoded: %5ld s\n",
          (clock() - t0) / CLOCKS_PER_SEC);

  fprintf(stderr, "[decode] n = %zu\n", clen);

  size_t errors = 0;
  for (size_t i = 0; i < n; i++) {
    errors += (au64[i] != decompressed[i]);
  }
  fprintf(stderr, "[decode] errors = %zu\n", errors);

  free(au64);
  free(compressed);
  free(decompressed);
}

void test_encdec_delta_manualfile(size_t n) {
  fprintf(stderr, "\n[%s]\n", __func__);
  fprintf(stderr, "[gen ] n = %zu\n", n);

  clock_t t0 = clock(), t1 = clock();
  uint64_t *au64 = malloc(n * sizeof au64[0]);
  for (size_t i = 0; i < n; i++) {
    au64[i] |= rand();
    au64[i] = (au64[i] << 32) | rand();
  }
  fprintf(stderr, "[gen ] generated: %5ld s\n",
          (clock() - t0) / CLOCKS_PER_SEC);

  t0 = clock();
  size_t compressed_len = 0;
  uint8_t *compressed = vb64_compress_delta_wl(au64, n, &compressed_len);
  t1 = clock();
  FILE *fcomp = fopen("compressed.bin", "w");
  fwrite(compressed, sizeof(uint8_t), compressed_len, fcomp);
  fclose(fcomp);
  fprintf(stderr, "[encode] encoded: %5ld s - dump: %5ld s\n",
          (t1 - t0) / CLOCKS_PER_SEC, (clock() - t1) / CLOCKS_PER_SEC);

  t0 = clock();

  fcomp = fopen("compressed.bin", "r");
  // get size of file
  struct stat sb;
  if (stat("compressed.bin", &sb) == -1) {
    fprintf(stderr, "Stat error, exit.\n");
    exit(EXIT_FAILURE);
  }
  // allocate and read file
  uint8_t *fcompressed = malloc(sb.st_size + 1);
  const size_t read_ = fread(fcompressed, sizeof(uint8_t), sb.st_size, fcomp);
  // check correct reading
  if (read_ != sb.st_size)
    exit(EXIT_FAILURE);
  fclose(fcomp);

  t1 = clock();
  size_t clen = 0;
  uint64_t *decompressed = vb64_decompress_delta_wl(fcompressed, &clen);
  fprintf(stderr, "[decode] decoded: %5ld s - load: %5ld s\n",
          (clock() - t1) / CLOCKS_PER_SEC, (t0 - t1) / CLOCKS_PER_SEC);

  fprintf(stderr, "[decode] n = %zu\n", clen);
  size_t errors = 0;
  for (size_t i = 0; i < n; i++) {
    errors += (au64[i] != decompressed[i]);
  }
  fprintf(stderr, "[decode] errors = %zu\n", errors);

  free(au64);
  free(compressed);
  free(decompressed);
}

void test_encdec_manualfile(size_t n) {
  fprintf(stderr, "\n[%s]\n", __func__);
  fprintf(stderr, "[gen ] n = %zu\n", n);

  clock_t t0 = clock(), t1 = clock();
  uint64_t *au64 = malloc(n * sizeof au64[0]);
  for (size_t i = 0; i < n; i++) {
    au64[i] |= rand();
    au64[i] = (au64[i] << 32) | rand();
  }
  fprintf(stderr, "[gen ] generated: %5ld s\n",
          (clock() - t0) / CLOCKS_PER_SEC);

  t0 = clock();
  size_t compressed_len = 0;
  uint8_t *compressed = vb64_compress_wl(au64, n, &compressed_len);
  t1 = clock();
  FILE *fcomp = fopen("compressed.bin", "w");
  fwrite(compressed, sizeof(uint8_t), compressed_len, fcomp);
  fclose(fcomp);
  fprintf(stderr, "[encode] encoded: %5ld s - dump: %5ld s\n",
          (t1 - t0) / CLOCKS_PER_SEC, (clock() - t1) / CLOCKS_PER_SEC);

  t0 = clock();

  fcomp = fopen("compressed.bin", "r");
  // get size of file
  struct stat sb;
  if (stat("compressed.bin", &sb) == -1) {
    fprintf(stderr, "Stat error, exit.\n");
    exit(EXIT_FAILURE);
  }
  // allocate and read file
  uint8_t *fcompressed = malloc(sb.st_size + 1);
  const size_t read_ = fread(fcompressed, sizeof(uint8_t), sb.st_size, fcomp);
  // check correct reading
  if (read_ != sb.st_size)
    exit(EXIT_FAILURE);
  fclose(fcomp);

  t1 = clock();
  size_t clen = 0;
  uint64_t *decompressed = vb64_decompress_wl(fcompressed, &clen);
  fprintf(stderr, "[decode] decoded: %5ld s - load: %5ld s\n",
          (clock() - t1) / CLOCKS_PER_SEC, (t0 - t1) / CLOCKS_PER_SEC);

  fprintf(stderr, "[decode] n = %zu\n", clen);
  size_t errors = 0;
  for (size_t i = 0; i < n; i++) {
    errors += (au64[i] != decompressed[i]);
  }
  fprintf(stderr, "[decode] errors = %zu\n", errors);

  free(au64);
  free(compressed);
  free(decompressed);
}

void test_encdec_delta_autofile(size_t n) {
  fprintf(stderr, "\n[%s]\n", __func__);
  fprintf(stderr, "[gen ] n = %zu\n", n);

  clock_t t0 = clock(), t1 = clock();
  uint64_t *au64 = malloc(n * sizeof au64[0]);
  for (size_t i = 0; i < n; i++) {
    au64[i] |= rand();
    au64[i] = (au64[i] << 32) | rand();
  }
  fprintf(stderr, "[gen ] generated: %5ld s\n",
          (clock() - t0) / CLOCKS_PER_SEC);

  t0 = clock();
  size_t compressed_len = 0;
  uint8_t *compressed = vb64_compress_delta_wl(au64, n, &compressed_len);
  t1 = clock();
  FILE *fcomp = fopen("compressed.bin", "w");
  fwrite(compressed, sizeof(uint8_t), compressed_len, fcomp);
  fclose(fcomp);
  fprintf(stderr, "[encode] encoded: %5ld s - dump: %5ld s\n",
          (t1 - t0) / CLOCKS_PER_SEC, (clock() - t1) / CLOCKS_PER_SEC);

  t0 = clock();
  size_t clen = 0;
  uint64_t *decompressed = vb64f_decompress_delta("compressed.bin", &clen);
  fprintf(stderr, "[decode] decoded form file: %5ld s\n",
          (clock() - t1) / CLOCKS_PER_SEC);

  fprintf(stderr, "[decode] n = %zu\n", clen);
  size_t errors = 0;
  for (size_t i = 0; i < n; i++) {
    errors += (au64[i] != decompressed[i]);
  }
  fprintf(stderr, "[decode] errors = %zu\n", errors);

  free(au64);
  free(compressed);
  free(decompressed);
}

int main(int argc, char *argv[]) {
  size_t test_size = 5e5;
  // test_encdec_delta(test_size);
  // test_encdec_delta_manualfile(test_size);
  // test_encdec(test_size);
  // test_encdec_manualfile(test_size);
  // test_encdec_delta_autofile(test_size);

  // sanity_check();
  // sanity_check_wl();
  sanity_check_file();
  return EXIT_SUCCESS;
}
