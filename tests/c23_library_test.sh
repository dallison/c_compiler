#!/bin/bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: $0 <davecc> <interpreter> <libc>" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
INTERPRETER="$ROOT/$2"
LIBC="$ROOT/$3"
INCLUDE_DIR="$ROOT/libc/include"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/c23-library.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

compile_source() {
  local name="$1"
  local standard="$2"
  local source="$3"
  local src="$WORK/$name.c"
  printf '%s\n' "$source" >"$src"
  "$DAVECC" -target pcode -std="$standard" -S -isystem "$INCLUDE_DIR" \
      "$src" -o "$WORK/$name.s" >"$WORK/$name.out" 2>&1
}

expect_compile() {
  local name="$1"
  local standard="$2"
  local source="$3"
  if ! compile_source "$name" "$standard" "$source"; then
    echo "$name: expected compilation success" >&2
    sed 's/^/  /' "$WORK/$name.out" >&2
    exit 1
  fi
  if [[ -s "$WORK/$name.out" ]]; then
    echo "$name: unexpected diagnostics" >&2
    sed 's/^/  /' "$WORK/$name.out" >&2
    exit 1
  fi
}

expect_compile c23_library_surface c23 \
  '#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static_assert(__STDC_VERSION_STRING_H__ == 202311L);
static_assert(__STDC_VERSION_TIME_H__ == 202311L);
static_assert(sizeof(PRIb64) > 1 && sizeof(PRIB64) > 1);
static_assert(sizeof(SCNb64) > 1);
static_assert(sizeof(PRIbFAST32) > 1 && sizeof(SCNbFAST32) > 1);
char mutable_text[4];
const char constant_text[4] = "abc";
int values[2];
const int constant_values[2] = {1, 2};
int compare_ints(const void *left, const void *right) {
  return *(const int *)left - *(const int *)right;
}
static_assert(_Generic(strchr(mutable_text, 0), char *: 1, default: 0));
static_assert(_Generic(strchr(constant_text, 0), const char *: 1, default: 0));
static_assert(_Generic(strpbrk(mutable_text, "a"), char *: 1, default: 0));
static_assert(_Generic(strpbrk(constant_text, "a"),
                       const char *: 1, default: 0));
static_assert(_Generic(strrchr(mutable_text, 0), char *: 1, default: 0));
static_assert(_Generic(strrchr(constant_text, 0),
                       const char *: 1, default: 0));
static_assert(_Generic(strstr(mutable_text, "a"), char *: 1, default: 0));
static_assert(_Generic(strstr(constant_text, "a"),
                       const char *: 1, default: 0));
static_assert(_Generic(memchr(values, 0, sizeof(values)), void *: 1, default: 0));
static_assert(_Generic(memchr(constant_values, 0, sizeof(constant_values)),
                       const void *: 1, default: 0));
static_assert(_Generic(bsearch(values, values, 2, sizeof(int), compare_ints),
                       void *: 1, default: 0));
static_assert(_Generic(bsearch(values, constant_values, 2, sizeof(int),
                               compare_ints), const void *: 1, default: 0));
void never_returns(void) { unreachable(); }
int main(void) {
  char bytes[8];
  struct timespec resolution;
  (void)memccpy(bytes, "abc", 0, 3);
  (void)memset_explicit(bytes, 0, sizeof(bytes));
  char *copy = strndup("abc", 2);
  free_sized(copy, 3);
  return timespec_getres(&resolution, TIME_UTC) == TIME_UTC ? 0 : 1;
}'

expect_compile c17_library_versions_absent c17 \
  '#include <string.h>
#include <time.h>
#ifdef __STDC_VERSION_STRING_H__
#error string version macro leaked before C23
#endif
#ifdef __STDC_VERSION_TIME_H__
#error time version macro leaked before C23
#endif
#ifdef strchr
#error const-preserving strchr macro leaked before C23
#endif
#ifdef bsearch
#error const-preserving bsearch macro leaked before C23
#endif
int main(void) { return 0; }'

runtime_source="$WORK/c23_library_runtime.c"
cat >"$runtime_source" <<'EOF'
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int SameCivilTime(const struct tm* left, const struct tm* right) {
  return left->tm_sec == right->tm_sec &&
         left->tm_min == right->tm_min &&
         left->tm_hour == right->tm_hour &&
         left->tm_mday == right->tm_mday &&
         left->tm_mon == right->tm_mon &&
         left->tm_year == right->tm_year;
}

int main(void) {
  char destination[8] = {'x', 'x', 'x', 'x', 'x', 'x', 'x', '\0'};
  if (memccpy(destination, "abc", 'b', 3) != destination + 2) return 1;
  if (destination[0] != 'a' || destination[1] != 'b' ||
      destination[2] != 'x') return 2;
  if (memccpy(destination, "abc", 'z', 3) != NULL) return 3;
  if (memccpy(destination, "abc", 'a' + 256, 3) != destination + 1) return 4;

  memset_explicit(destination, 0xa5, 7);
  for (int i = 0; i < 7; ++i) {
    if ((unsigned char)destination[i] != 0xa5) return 5;
  }

  char* whole = strdup("DaveCC");
  char* prefix = strndup("compiler", 4);
  char* short_string = strndup("C23", 20);
  if (whole == NULL || prefix == NULL || short_string == NULL) return 6;
  if (strcmp(whole, "DaveCC") != 0 || strcmp(prefix, "comp") != 0 ||
      strcmp(short_string, "C23") != 0) return 7;
  free_sized(whole, 7);
  free(prefix);
  free(short_string);
  free_sized(NULL, 123);
  free_aligned_sized(NULL, 64, 128);

  char buffer[96];
  if (snprintf(buffer, sizeof(buffer), "%b", 10U) != 4 ||
      strcmp(buffer, "1010") != 0) return 8;
  if (snprintf(buffer, sizeof(buffer), "%#010b", 5U) != 10 ||
      strcmp(buffer, "0b00000101") != 0) return 9;
  if (snprintf(buffer, sizeof(buffer), "%#B", 5U) != 5 ||
      strcmp(buffer, "0B101") != 0) return 10;
  if (snprintf(buffer, sizeof(buffer), "%.0b", 0U) != 0 ||
      strcmp(buffer, "") != 0) return 11;
  if (snprintf(buffer, sizeof(buffer), "%llb",
               (unsigned long long)0x8000000000000001ULL) != 64 ||
      buffer[0] != '1' || buffer[63] != '1' || buffer[64] != '\0') return 12;
  char small_buffer[4];
  if (snprintf(small_buffer, sizeof(small_buffer), "%b", 15U) != 4 ||
      small_buffer[0] != '1' || small_buffer[1] != '1' ||
      small_buffer[2] != '1' || small_buffer[3] != '\0') return 31;

  uint64_t printed = 9;
  if (snprintf(buffer, sizeof(buffer), "%#" PRIb64, printed) != 6 ||
      strcmp(buffer, "0b1001") != 0) return 13;
  if (snprintf(buffer, sizeof(buffer), "%#" PRIB64, printed) != 6 ||
      strcmp(buffer, "0B1001") != 0) return 14;

  unsigned int binary = 0;
  unsigned long long wide_binary = 0;
  if (sscanf("0b101101", "%b", &binary) != 1 || binary != 45) return 15;
  if (sscanf("101", "%2b", &binary) != 1 || binary != 2) return 16;
  if (sscanf("0B10000000000000001", "%llb", &wide_binary) != 1 ||
      wide_binary != 65537ULL) return 17;
  if (sscanf("1111", "%" SCNb64, &printed) != 1 || printed != 15) return 18;
  int autodetected = 0;
  if (sscanf("0b101", "%i", &autodetected) != 1 ||
      autodetected != 5) return 32;
  if (sscanf("0123", "%i", &autodetected) != 1 ||
      autodetected != 83) return 33;

  char* conversion_end = NULL;
  const char binary_number[] = "0b101";
  if (strtol(binary_number, &conversion_end, 0) != 5 ||
      *conversion_end != '\0') return 34;
  if (strtol("-0B101", &conversion_end, 2) != -5 ||
      *conversion_end != '\0') return 35;
  const char incomplete_binary[] = "0b";
  if (strtol(incomplete_binary, &conversion_end, 0) != 0 ||
      conversion_end != incomplete_binary + 1) return 36;
  if (strtoul(binary_number, &conversion_end, 0) != 5 ||
      strtoll(binary_number, &conversion_end, 0) != 5 ||
      strtoull(binary_number, &conversion_end, 2) != 5) return 37;

  if (snprintf(buffer, sizeof(buffer), "%w8d", (int8_t)-1) != 2 ||
      strcmp(buffer, "-1") != 0) return 38;
  if (snprintf(buffer, sizeof(buffer), "%w8u", (uint8_t)255) != 3 ||
      strcmp(buffer, "255") != 0) return 39;
  if (snprintf(buffer, sizeof(buffer), "%w16x", (uint16_t)0x2345) != 4 ||
      strcmp(buffer, "2345") != 0) return 40;
  if (snprintf(buffer, sizeof(buffer), "%w32d", (int32_t)-123456) != 7 ||
      strcmp(buffer, "-123456") != 0) return 41;
  if (snprintf(buffer, sizeof(buffer), "%wf64u",
               (uint_fast64_t)18446744073709551615ULL) != 20 ||
      strcmp(buffer, "18446744073709551615") != 0) return 42;
  int8_t scanned8 = 0;
  uint32_t scanned32 = 0;
  if (sscanf("-128", "%w8d", &scanned8) != 1 || scanned8 != -128) return 43;
  if (sscanf("ffffffff", "%w32x", &scanned32) != 1 ||
      scanned32 != 0xffffffffU) return 44;
  int8_t count8 = 0;
  if (sscanf("17", "%w8d%w8n", &scanned8, &count8) != 1 ||
      scanned8 != 17 || count8 != 2) return 45;
  if (snprintf(buffer, sizeof(buffer), "%w128d", 1) >= 0) return 46;
  if (sscanf("1", "%w128d", &scanned32) != 0) return 47;

  struct timespec resolution;
  struct timespec now;
  if (timespec_getres(&resolution, TIME_UTC) != TIME_UTC) return 19;
  if (resolution.tv_sec != 0 || resolution.tv_nsec <= 0 ||
      resolution.tv_nsec >= 1000000000) return 20;
  if (timespec_getres(NULL, TIME_UTC) != TIME_UTC ||
      timespec_getres(&resolution, -1) != 0) return 21;
  if (timespec_get(&now, TIME_UTC) != TIME_UTC ||
      now.tv_nsec < 0 || now.tv_nsec >= 1000000000) return 22;

  time_t epoch = 0;
  struct tm utc;
  if (gmtime_r(&epoch, &utc) != &utc) return 23;
  if (utc.tm_year != 70 || utc.tm_mon != 0 || utc.tm_mday != 1 ||
      utc.tm_hour != 0 || utc.tm_min != 0 || utc.tm_sec != 0 ||
      utc.tm_wday != 4 || utc.tm_yday != 0 || utc.tm_gmtoff != 0 ||
      strcmp(utc.tm_zone, "UTC") != 0) return 24;

  time_t before_epoch = -1;
  if (gmtime_r(&before_epoch, &utc) == NULL) return 25;
  if (utc.tm_year != 69 || utc.tm_mon != 11 || utc.tm_mday != 31 ||
      utc.tm_hour != 23 || utc.tm_min != 59 || utc.tm_sec != 59 ||
      utc.tm_wday != 3 || utc.tm_yday != 364) return 26;

  time_t leap_day = 951782400;
  if (gmtime_r(&leap_day, &utc) == NULL) return 27;
  if (utc.tm_year != 100 || utc.tm_mon != 1 || utc.tm_mday != 29 ||
      utc.tm_yday != 59 || utc.tm_wday != 2) return 28;

  struct tm local;
  struct tm offset_utc;
  if (localtime_r(&epoch, &local) == NULL || local.tm_zone == NULL) return 29;
  time_t adjusted = epoch + local.tm_gmtoff;
  if (gmtime_r(&adjusted, &offset_utc) == NULL ||
      !SameCivilTime(&local, &offset_utc)) return 30;
  return 0;
}
EOF

for target in pcode x86_64 aarch64 arm riscv 6502; do
  "$DAVECC" -target "$target" -std=c23 -O0 -S -isystem "$INCLUDE_DIR" \
      "$runtime_source" -o "$WORK/c23_library_runtime.$target.s"
done

"$DAVECC" -target x86_64 -std=c23 -O1 -static -isystem "$INCLUDE_DIR" \
    -Wl,-e -Wl,main "$runtime_source" "$LIBC" -o "$WORK/c23_library_runtime.exe"
"$INTERPRETER" -i "$WORK/c23_library_runtime.exe"
