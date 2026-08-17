#ifndef _STDATOMIC_H
#define _STDATOMIC_H

#if defined(__cplusplus)
#error "<stdatomic.h> is a C header; use <atomic> in C++"
#elif !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L
#error "<stdatomic.h> requires C11 or later"
#elif defined(__STDC_NO_ATOMICS__)
#error "<stdatomic.h> is unavailable on this target"
#else

#include <stddef.h>
#include <stdint.h>

typedef enum {
  memory_order_relaxed = __ATOMIC_RELAXED,
  memory_order_consume = __ATOMIC_CONSUME,
  memory_order_acquire = __ATOMIC_ACQUIRE,
  memory_order_release = __ATOMIC_RELEASE,
  memory_order_acq_rel = __ATOMIC_ACQ_REL,
  memory_order_seq_cst = __ATOMIC_SEQ_CST,
} memory_order;

typedef _Atomic _Bool atomic_bool;
typedef _Atomic char atomic_char;
typedef _Atomic signed char atomic_schar;
typedef _Atomic unsigned char atomic_uchar;
typedef _Atomic short atomic_short;
typedef _Atomic unsigned short atomic_ushort;
typedef _Atomic int atomic_int;
typedef _Atomic unsigned int atomic_uint;
typedef _Atomic long atomic_long;
typedef _Atomic unsigned long atomic_ulong;
typedef _Atomic long long atomic_llong;
typedef _Atomic unsigned long long atomic_ullong;
typedef _Atomic unsigned short atomic_char16_t;
typedef _Atomic unsigned int atomic_char32_t;
typedef _Atomic wchar_t atomic_wchar_t;
typedef _Atomic int_least8_t atomic_int_least8_t;
typedef _Atomic uint_least8_t atomic_uint_least8_t;
typedef _Atomic int_least16_t atomic_int_least16_t;
typedef _Atomic uint_least16_t atomic_uint_least16_t;
typedef _Atomic int_least32_t atomic_int_least32_t;
typedef _Atomic uint_least32_t atomic_uint_least32_t;
typedef _Atomic int_least64_t atomic_int_least64_t;
typedef _Atomic uint_least64_t atomic_uint_least64_t;
typedef _Atomic int_fast8_t atomic_int_fast8_t;
typedef _Atomic uint_fast8_t atomic_uint_fast8_t;
typedef _Atomic int_fast16_t atomic_int_fast16_t;
typedef _Atomic uint_fast16_t atomic_uint_fast16_t;
typedef _Atomic int_fast32_t atomic_int_fast32_t;
typedef _Atomic uint_fast32_t atomic_uint_fast32_t;
typedef _Atomic int_fast64_t atomic_int_fast64_t;
typedef _Atomic uint_fast64_t atomic_uint_fast64_t;
typedef _Atomic intptr_t atomic_intptr_t;
typedef _Atomic uintptr_t atomic_uintptr_t;
typedef _Atomic size_t atomic_size_t;
typedef _Atomic ptrdiff_t atomic_ptrdiff_t;
typedef _Atomic intmax_t atomic_intmax_t;
typedef _Atomic uintmax_t atomic_uintmax_t;

typedef _Atomic _Bool atomic_flag;

#define ATOMIC_VAR_INIT(value) (value)
#define ATOMIC_FLAG_INIT 0

#define ATOMIC_BOOL_LOCK_FREE 2
#define ATOMIC_CHAR_LOCK_FREE 2
#define ATOMIC_CHAR16_T_LOCK_FREE 2
#define ATOMIC_CHAR32_T_LOCK_FREE 2
#define ATOMIC_WCHAR_T_LOCK_FREE 2
#define ATOMIC_SHORT_LOCK_FREE 2
#define ATOMIC_INT_LOCK_FREE 2
#define ATOMIC_LONG_LOCK_FREE 2
#if defined(__arm__)
#define ATOMIC_LLONG_LOCK_FREE 0
#else
#define ATOMIC_LLONG_LOCK_FREE 2
#endif
#define ATOMIC_POINTER_LOCK_FREE 2

#define kill_dependency(value) (value)

#define atomic_init(object, desired) \
  __atomic_store_n((object), (desired), __ATOMIC_RELAXED)

#define atomic_is_lock_free(object) \
  (sizeof(*(object)) <= sizeof(void*) ? 1 : 0)

#define atomic_store_explicit(object, desired, order) \
  __atomic_store_n((object), (desired), (order))
#define atomic_store(object, desired) \
  atomic_store_explicit((object), (desired), memory_order_seq_cst)

#define atomic_load_explicit(object, order) \
  __atomic_load_n((object), (order))
#define atomic_load(object) \
  atomic_load_explicit((object), memory_order_seq_cst)

#define atomic_compare_exchange_strong_explicit( \
    object, expected, desired, success, failure) \
  __atomic_compare_exchange_n((object), (expected), (desired), 0, \
                              (success), (failure))
#define atomic_compare_exchange_weak_explicit( \
    object, expected, desired, success, failure) \
  __atomic_compare_exchange_n((object), (expected), (desired), 1, \
                              (success), (failure))
#define atomic_compare_exchange_strong(object, expected, desired) \
  atomic_compare_exchange_strong_explicit( \
      (object), (expected), (desired), memory_order_seq_cst, \
      memory_order_seq_cst)
#define atomic_compare_exchange_weak(object, expected, desired) \
  atomic_compare_exchange_weak_explicit( \
      (object), (expected), (desired), memory_order_seq_cst, \
      memory_order_seq_cst)

#define atomic_fetch_add_explicit(object, operand, order) \
  __atomic_fetch_add((object), (operand), (order))
#define atomic_fetch_sub_explicit(object, operand, order) \
  __atomic_fetch_sub((object), (operand), (order))
#define atomic_fetch_add(object, operand) \
  atomic_fetch_add_explicit((object), (operand), memory_order_seq_cst)
#define atomic_fetch_sub(object, operand) \
  atomic_fetch_sub_explicit((object), (operand), memory_order_seq_cst)

#define atomic_thread_fence(order) __atomic_thread_fence(order)
#define atomic_signal_fence(order) __atomic_signal_fence(order)

static __attribute__((unused)) _Bool
__davecc_atomic_flag_test_and_set_explicit(
    atomic_flag* object, memory_order order) {
  _Bool expected = 0;
  while (!__atomic_compare_exchange_n(object, &expected, 1, 0, order,
                                      memory_order_relaxed)) {
    if (expected) {
      return 1;
    }
    expected = 0;
  }
  return 0;
}

static __attribute__((unused)) void
__davecc_atomic_flag_clear_explicit(atomic_flag* object,
                                    memory_order order) {
  __atomic_store_n(object, 0, order);
}

#define atomic_flag_test_and_set_explicit(object, order) \
  __davecc_atomic_flag_test_and_set_explicit((atomic_flag*)(object), (order))
#define atomic_flag_test_and_set(object) \
  atomic_flag_test_and_set_explicit((object), memory_order_seq_cst)
#define atomic_flag_clear_explicit(object, order) \
  __davecc_atomic_flag_clear_explicit((atomic_flag*)(object), (order))
#define atomic_flag_clear(object) \
  atomic_flag_clear_explicit((object), memory_order_seq_cst)

#endif
#endif
