/*
 * Minimal crypt(3) for DaveOS password hashing.
 */
#include <crypt.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

static unsigned long Fnv1a(const char* a, const char* b) {
  unsigned long h = 2166136261ul;
  const char* p;
  for (p = a; p != NULL && *p != '\0'; p++) {
    h ^= (unsigned char)*p;
    h *= 16777619ul;
  }
  for (p = b; p != NULL && *p != '\0'; p++) {
    h ^= (unsigned char)*p;
    h *= 16777619ul;
  }
  return h;
}

static void Hex8(unsigned long v, char* out) {
  static const char* digits = "0123456789abcdef";
  int i;
  for (i = 7; i >= 0; i--) {
    out[i] = digits[v & 0xfu];
    v >>= 4;
  }
  out[8] = '\0';
}

char* crypt(const char* key, const char* setting) {
  static char out[32];
  char salt[9];
  unsigned long h;
  if (key == NULL) {
    key = "";
  }
  if (setting != NULL && setting[0] == '$' && setting[1] == 'd' &&
      setting[2] == '$' && strlen(setting) >= 11) {
    memcpy(salt, setting + 3, 8);
    salt[8] = '\0';
  } else {
    Hex8((unsigned long)time(NULL) ^ (unsigned long)(uintptr_t)key, salt);
  }
  h = Fnv1a(salt, key);
  out[0] = '$';
  out[1] = 'd';
  out[2] = '$';
  memcpy(out + 3, salt, 8);
  out[11] = '$';
  Hex8(h, out + 12);
  return out;
}
