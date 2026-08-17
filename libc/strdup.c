#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static size_t BoundedStringLength(const char* string, size_t limit) {
  size_t length = 0;
  while (length < limit && string[length] != '\0') {
    ++length;
  }
  return length;
}

static char* DuplicateString(const char* string, size_t length) {
  char* copy = malloc(length + 1);
  if (copy == NULL) {
    return NULL;
  }
  if (length != 0) {
    memcpy(copy, string, length);
  }
  copy[length] = '\0';
  return copy;
}

char* strndup(const char* string, size_t limit) {
  return DuplicateString(string, BoundedStringLength(string, limit));
}

char* strdup(const char* string) {
  return DuplicateString(string, strlen(string));
}
