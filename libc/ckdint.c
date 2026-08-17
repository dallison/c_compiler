#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint64_t magnitude;
  bool negative;
  bool wider_than_uint64;
} CheckedInteger;

static CheckedInteger CheckedIntegerFromValue(uint64_t value,
                                              bool is_signed) {
  CheckedInteger converted;
  converted.negative = is_signed && (int64_t)value < 0;
  converted.magnitude = converted.negative ? (uint64_t)0 - value : value;
  converted.wider_than_uint64 = false;
  return converted;
}

static CheckedInteger CheckedIntegerAdd(CheckedInteger left,
                                        CheckedInteger right) {
  CheckedInteger result;
  result.wider_than_uint64 = false;
  if (left.negative == right.negative) {
    result.magnitude = left.magnitude + right.magnitude;
    result.negative = left.negative;
    result.wider_than_uint64 = result.magnitude < left.magnitude;
  } else if (left.magnitude >= right.magnitude) {
    result.magnitude = left.magnitude - right.magnitude;
    result.negative = left.negative;
  } else {
    result.magnitude = right.magnitude - left.magnitude;
    result.negative = right.negative;
  }
  if (result.magnitude == 0) {
    result.negative = false;
  }
  return result;
}

static CheckedInteger CheckedIntegerMultiply(CheckedInteger left,
                                             CheckedInteger right) {
  CheckedInteger result;
  result.negative = left.negative != right.negative;
  result.wider_than_uint64 =
      left.magnitude != 0 &&
      right.magnitude > ~(uint64_t)0 / left.magnitude;
  result.magnitude = left.magnitude * right.magnitude;
  if (result.magnitude == 0 && !result.wider_than_uint64) {
    result.negative = false;
  }
  return result;
}

static bool CheckedIntegerStore(volatile void* destination, size_t size,
                                bool is_signed, CheckedInteger value) {
  unsigned int width = (unsigned int)(size * 8);
  bool overflow = value.wider_than_uint64;
  if (!overflow && value.negative) {
    if (!is_signed) {
      overflow = value.magnitude != 0;
    } else {
      uint64_t limit =
          width == 64 ? (uint64_t)1 << 63 : (uint64_t)1 << (width - 1);
      overflow = value.magnitude > limit;
    }
  } else if (!overflow) {
    uint64_t limit;
    if (is_signed) {
      limit = width == 64 ? (~(uint64_t)0 >> 1)
                          : ((uint64_t)1 << (width - 1)) - 1;
    } else {
      limit = width == 64 ? ~(uint64_t)0 : ((uint64_t)1 << width) - 1;
    }
    overflow = value.magnitude > limit;
  }

  uint64_t representation =
      value.negative ? (uint64_t)0 - value.magnitude : value.magnitude;
  switch (size) {
    case 1:
      *(volatile uint8_t*)destination = (uint8_t)representation;
      break;
    case 2:
      *(volatile uint16_t*)destination = (uint16_t)representation;
      break;
    case 4:
      *(volatile uint32_t*)destination = (uint32_t)representation;
      break;
    case 8:
      *(volatile uint64_t*)destination = representation;
      break;
  }
  return overflow;
}

bool __davecc_ckd_add(volatile void* result, size_t result_size,
                      bool result_is_signed, uint64_t left,
                      bool left_is_signed, uint64_t right,
                      bool right_is_signed) {
  CheckedInteger checked = CheckedIntegerAdd(
      CheckedIntegerFromValue(left, left_is_signed),
      CheckedIntegerFromValue(right, right_is_signed));
  return CheckedIntegerStore(result, result_size, result_is_signed, checked);
}

bool __davecc_ckd_sub(volatile void* result, size_t result_size,
                      bool result_is_signed, uint64_t left,
                      bool left_is_signed, uint64_t right,
                      bool right_is_signed) {
  CheckedInteger lhs = CheckedIntegerFromValue(left, left_is_signed);
  CheckedInteger rhs = CheckedIntegerFromValue(right, right_is_signed);
  if (rhs.magnitude != 0) {
    rhs.negative = !rhs.negative;
  }
  return CheckedIntegerStore(result, result_size, result_is_signed,
                             CheckedIntegerAdd(lhs, rhs));
}

bool __davecc_ckd_mul(volatile void* result, size_t result_size,
                      bool result_is_signed, uint64_t left,
                      bool left_is_signed, uint64_t right,
                      bool right_is_signed) {
  CheckedInteger checked = CheckedIntegerMultiply(
      CheckedIntegerFromValue(left, left_is_signed),
      CheckedIntegerFromValue(right, right_is_signed));
  return CheckedIntegerStore(result, result_size, result_is_signed, checked);
}
