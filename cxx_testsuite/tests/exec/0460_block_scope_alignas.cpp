// RUN: -std=c++20
// EXPECT_EXIT: 0
//
// A block-scope alignment specifier must parse as a declaration and must
// raise the alignment of the local's frame slot.
//
// The checks live in a callee rather than in main: the exec harness enters at
// main with -Wl,-e -Wl,main, so main's frame does not get the entry alignment
// the ABI would otherwise guarantee.

using uintptr = __UINTPTR_TYPE__;

static bool aligned_to(const void *p, uintptr n) {
    return (reinterpret_cast<uintptr>(p) & (n - 1)) == 0;
}

struct alignas(16) Wide {
    char storage[16];
};

struct alignas(64) Constructed {
    int value;
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

    alignas(32) char a32[1];
    if (!aligned_to(a32, 32)) return 9;

    alignas(64) char a64[1];
    if (!aligned_to(a64, 64)) return 10;

    // Type-id form of the specifier.
    alignas(double) char td[1];
    if (!aligned_to(td, alignof(double))) return 5;

    // A class type carrying its own alignment.
    Wide w;
    if (!aligned_to(&w, 16)) return 6;

    // Alignment carried by a type must use the same dynamically aligned
    // storage even when the declaration itself has no alignas specifier.
    Constructed constructed = {27};
    if (!aligned_to(&constructed, 64) || constructed.value != 27) return 11;

    // A weaker request must not lower the natural alignment.
    alignas(1) long weak = 0;
    if (!aligned_to(&weak, alignof(long))) return 7;

    // alignas on a block-scope static, which is laid out as a data-section
    // object rather than a frame slot.
    alignas(16) static char st[1];
    if (!aligned_to(st, 16)) return 8;

    return 0;
}

__attribute__((noinline)) static int repeated_scope() {
    uintptr first = 0;
    for (int i = 0; i < 32; ++i) {
        alignas(64) char local[7];
        uintptr address = reinterpret_cast<uintptr>(local);
        if (i == 0) {
            first = address;
        } else if (address != first) {
            return 12;
        }
        local[0] = static_cast<char>(i);
    }
    return 0;
}

__attribute__((noinline)) static int goto_out_of_scope() {
    uintptr first = 0;
    {
        alignas(64) char local[7];
        first = reinterpret_cast<uintptr>(local);
        goto after_first;
    }
after_first:
    {
        alignas(64) char local[7];
        if (reinterpret_cast<uintptr>(local) != first) return 13;
    }
    return 0;
}

__attribute__((noinline)) static int mixed_with_vla(int n) {
    char vla[n];
    vla[0] = 3;
    alignas(64) char local[5];
    local[0] = 4;
    return aligned_to(local, 64) ? vla[0] + local[0] : 14;
}

int main() {
    int result = check();
    if (result != 0) return result;
    result = repeated_scope();
    if (result != 0) return result;
    result = goto_out_of_scope();
    if (result != 0) return result;
    return mixed_with_vla(5) == 7 ? 0 : 15;
}
