// RUN: -std=c++29

#include <cstdint>
#include <type_traits>

#if !defined(__6502__) && !defined(__W65C02__)
#include <atomic>
#endif

static_assert(sizeof(std::intptr_t) >= sizeof(void*));
static_assert(sizeof(std::uintptr_t) >= sizeof(void*));
static_assert(std::is_signed_v<std::intptr_t>);
static_assert(std::is_unsigned_v<std::uintptr_t>);

static_assert(INTPTR_MIN == -INTPTR_MAX - 1);
static_assert(UINTPTR_MAX == static_cast<std::uintptr_t>(-1));
static_assert(
    std::is_same_v<decltype(INTPTR_MAX), std::intptr_t>);
static_assert(
    std::is_same_v<decltype(UINTPTR_MAX), std::uintptr_t>);

#if !defined(__6502__) && !defined(__W65C02__)
static_assert(
    std::is_same_v<std::atomic_intptr_t, std::atomic<std::intptr_t>>);
static_assert(
    std::is_same_v<std::atomic_uintptr_t, std::atomic<std::uintptr_t>>);
#endif
