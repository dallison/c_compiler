// RUN: -std=c++20
// EXPECT_EXIT: 0
//
// A block-scope alignment specifier must parse as a declaration and must
// raise the alignment of the local's frame slot.
//
// The checks live in a callee rather than in main: the exec harness enters at
// main with -Wl,-e -Wl,main, so main's frame does not get the entry alignment
// the ABI would otherwise guarantee.

static bool aligned_to(const void *p, unsigned long n) {
    return (reinterpret_cast<unsigned long>(p) & (n - 1)) == 0;
}

struct alignas(16) Wide {
    char storage[16];
};

__attribute__((noinline)) static int check() {
    // An odd-sized declaration first, so a correct result cannot come from the
    // slots happening to be laid out at aligned offsets anyway.
    char filler[3];
    filler[0] = 0;

    alignas(2) char a2[1];
    if (!aligned_to(a2, 2)) return 1;

    alignas(4) char a4[1];
    if (!aligned_to(a4, 4)) return 2;

    alignas(8) char a8[1];
    if (!aligned_to(a8, 8)) return 3;

    alignas(16) char a16[1];
    if (!aligned_to(a16, 16)) return 4;

    // Type-id form of the specifier.
    alignas(double) char td[1];
    if (!aligned_to(td, alignof(double))) return 5;

    // A class type carrying its own alignment.
    Wide w;
    if (!aligned_to(&w, 16)) return 6;

    // A weaker request must not lower the natural alignment.
    alignas(1) long weak = 0;
    if (!aligned_to(&weak, alignof(long))) return 7;

    // alignas on a block-scope static, which is laid out as a data-section
    // object rather than a frame slot.
    alignas(16) static char st[1];
    if (!aligned_to(st, 16)) return 8;

    return static_cast<int>(filler[0]) + static_cast<int>(weak);
}

int main() { return check(); }
