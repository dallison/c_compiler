#ifndef checked_math_h
#define checked_math_h

#include <stdbool.h>
#include <stdint.h>

static bool Int64AddOverflow(int64_t left, int64_t right, int64_t* result) {
  if ((right > 0 && left > INT64_MAX - right) ||
      (right < 0 && left < INT64_MIN - right)) {
    return true;
  }
  *result = left + right;
  return false;
}

static bool Int64SubOverflow(int64_t left, int64_t right, int64_t* result) {
  if ((right > 0 && left < INT64_MIN + right) ||
      (right < 0 && left > INT64_MAX + right)) {
    return true;
  }
  *result = left - right;
  return false;
}

static bool Int64MulOverflow(int64_t left, int64_t right, int64_t* result) {
  if (left == 0 || right == 0) {
    *result = 0;
    return false;
  }
  if ((left == -1 && right == INT64_MIN) ||
      (right == -1 && left == INT64_MIN)) {
    return true;
  }
  if (left > 0) {
    if ((right > 0 && left > INT64_MAX / right) ||
        (right < 0 && right < INT64_MIN / left)) {
      return true;
    }
  } else if ((right > 0 && left < INT64_MIN / right) ||
             (right < 0 && left < INT64_MAX / right)) {
    return true;
  }
  *result = left * right;
  return false;
}

#endif
