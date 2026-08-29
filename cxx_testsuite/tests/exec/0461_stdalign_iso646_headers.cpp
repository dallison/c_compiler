// RUN: -std=c++17
// EXPECT_EXIT: 0
//
// <ciso646>, <cstdalign> and <cstdbool> are contentless in C++ but must exist
// and must define their feature macros through C++17.

#include <ciso646>
#include <cstdalign>
#include <cstdbool>
#include <iso646.h>
#include <stdalign.h>

#ifndef __alignas_is_defined
#error "<cstdalign> must define __alignas_is_defined"
#endif
#ifndef __alignof_is_defined
#error "<cstdalign> must define __alignof_is_defined"
#endif
#ifndef __bool_true_false_are_defined
#error "<cstdbool> must define __bool_true_false_are_defined"
#endif

// The alternative spellings are keywords in C++, so <iso646.h> must not have
// turned them into macros that break this declaration.
struct and_eq_user {
    int bitand_;
};

__attribute__((noinline)) static int check() {
    alignas(8) char buf[1];
    if ((reinterpret_cast<unsigned long>(buf) & 7UL) != 0) return 1;
    if (alignof(double) == 0) return 2;

    bool t = true and true;
    bool f = true and not true;
    if (not t) return 3;
    if (f) return 4;
    if ((5 bitand 3) != 1) return 5;
    if ((5 bitor 2) != 7) return 6;
    if ((5 xor 1) != 4) return 7;
    if (1 not_eq 1) return 8;

    and_eq_user u{0};
    return u.bitand_;
}

int main() { return check(); }
