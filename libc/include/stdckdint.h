#ifndef _STDCKDINT_H
#define _STDCKDINT_H

#if defined(__cplusplus)
#error "<stdckdint.h> is a C header"
#elif !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
#error "<stdckdint.h> requires C23 or later"
#else

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define __STDC_VERSION_STDCKDINT_H__ 202311L

bool __davecc_ckd_add(volatile void* result, size_t result_size,
                      bool result_is_signed, uint64_t left,
                      bool left_is_signed, uint64_t right,
                      bool right_is_signed);
bool __davecc_ckd_sub(volatile void* result, size_t result_size,
                      bool result_is_signed, uint64_t left,
                      bool left_is_signed, uint64_t right,
                      bool right_is_signed);
bool __davecc_ckd_mul(volatile void* result, size_t result_size,
                      bool result_is_signed, uint64_t left,
                      bool left_is_signed, uint64_t right,
                      bool right_is_signed);

#define __DAVECC_CKD_VALUE(value)                                           \
  _Generic((value),                                                         \
      signed char: (uint64_t)(value),                                       \
      unsigned char: (uint64_t)(value),                                     \
      signed short: (uint64_t)(value),                                      \
      unsigned short: (uint64_t)(value),                                    \
      signed int: (uint64_t)(value),                                        \
      unsigned int: (uint64_t)(value),                                      \
      signed long: (uint64_t)(value),                                       \
      unsigned long: (uint64_t)(value),                                     \
      signed long long: (uint64_t)(value),                                  \
      unsigned long long: (uint64_t)(value))

#define __DAVECC_CKD_IS_SIGNED(value)                                       \
  _Generic((value),                                                         \
      signed char: true,                                                    \
      unsigned char: false,                                                 \
      signed short: true,                                                   \
      unsigned short: false,                                                \
      signed int: true,                                                     \
      unsigned int: false,                                                  \
      signed long: true,                                                    \
      unsigned long: false,                                                 \
      signed long long: true,                                               \
      unsigned long long: false)

#define __DAVECC_CKD_RESULT_SIGNED(result)                                  \
  _Generic(*(result),                                                       \
      signed char: true,                                                    \
      unsigned char: false,                                                 \
      signed short: true,                                                   \
      unsigned short: false,                                                \
      signed int: true,                                                     \
      unsigned int: false,                                                  \
      signed long: true,                                                    \
      unsigned long: false,                                                 \
      signed long long: true,                                               \
      unsigned long long: false)

#define __DAVECC_CKD_RESULT_SIZE(result)                                    \
  _Generic(*(result),                                                       \
      signed char: sizeof(signed char),                                     \
      unsigned char: sizeof(unsigned char),                                 \
      signed short: sizeof(signed short),                                   \
      unsigned short: sizeof(unsigned short),                               \
      signed int: sizeof(signed int),                                       \
      unsigned int: sizeof(unsigned int),                                   \
      signed long: sizeof(signed long),                                     \
      unsigned long: sizeof(unsigned long),                                 \
      signed long long: sizeof(signed long long),                           \
      unsigned long long: sizeof(unsigned long long))

#define __DAVECC_CKD_CALL(operation, result, left, right)                    \
  __davecc_ckd_##operation(                                                  \
      (result), __DAVECC_CKD_RESULT_SIZE(result),                            \
      __DAVECC_CKD_RESULT_SIGNED(result), __DAVECC_CKD_VALUE(left),          \
      __DAVECC_CKD_IS_SIGNED(left), __DAVECC_CKD_VALUE(right),              \
      __DAVECC_CKD_IS_SIGNED(right))

#define ckd_add(result, left, right)                                         \
  __DAVECC_CKD_CALL(add, result, left, right)
#define ckd_sub(result, left, right)                                         \
  __DAVECC_CKD_CALL(sub, result, left, right)
#define ckd_mul(result, left, right)                                         \
  __DAVECC_CKD_CALL(mul, result, left, right)

#endif

#endif /* _STDCKDINT_H */
