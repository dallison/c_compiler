// RUN: -std=c++17
// EXPECT_EXIT: 0
//
// A variable-length array must be readable and writable through its own
// storage.  The suite's only other VLA test declares one without ever
// dereferencing it, so an array based on an address that was never computed
// went unnoticed: the declaration's savesp built the move that reads the stack
// pointer but never handed it to Emit, leaving the array based wherever the
// register happened to point.
//
// The base and the pre-allocation stack pointer both outlive the expression
// that computes them. Keep them in hidden pointer-sized frame slots: temporary
// registers cannot safely span calls, loop back edges, or a second dynamic
// allocation.
//
// The checks live in callees because the exec harness enters at main.

using uintptr = __UINTPTR_TYPE__;

__attribute__((noinline)) static int single_array(int n) {
    char v[n];
    v[0] = 3;
    v[1] = 4;
    v[n - 1] = 5;
    if (sizeof(v) != static_cast<unsigned>(n)) return -1;
    if (reinterpret_cast<uintptr>(&v) != reinterpret_cast<uintptr>(v)) return -1;
    return v[0] + v[1] + v[n - 1];
}

__attribute__((noinline)) static int two_arrays(int n) {
    int a[n];
    int b[n];
    a[0] = 7;
    b[0] = 20;
    a[n - 1] = 11;
    b[n - 1] = 29;
    return (b[0] - a[0]) + (b[n - 1] - a[n - 1]);
}

__attribute__((noinline)) static int consume(int value) {
    return value * 2;
}

__attribute__((noinline)) static int across_a_call(int n) {
    int v[n];
    v[0] = 9;
    int result = consume(v[0]);
    v[n - 1] = result + 3;
    return v[0] + v[n - 1];
}

__attribute__((noinline)) static int repeated_scope(int n) {
    uintptr first = 0;
    int sum = 0;
    for (int iter = 0; iter < 64; ++iter) {
        char v[n];
        uintptr address = reinterpret_cast<uintptr>(v);
        if (iter == 0) {
            first = address;
        } else if (address != first) {
            return -1;  // The previous iteration did not restore sp.
        }
        v[0] = static_cast<char>(iter);
        v[n - 1] = static_cast<char>(iter + 3);
        sum += v[n - 1] - v[0];
    }
    return sum;
}

__attribute__((noinline)) static int nested_scope(int n) {
    int outer[n];
    outer[0] = 5;
    uintptr outer_address = reinterpret_cast<uintptr>(outer);
    {
        int inner[n];
        inner[0] = 8;
        outer[0] += inner[0];
    }
    if (reinterpret_cast<uintptr>(outer) != outer_address) return -1;
    outer[n - 1] = 4;
    return outer[0] + outer[n - 1];
}

int main() {
    if (single_array(4) != 12) return 1;
    if (two_arrays(3) != 31) return 2;
    if (across_a_call(4) != 30) return 3;
    if (repeated_scope(5) != 64 * 3) return 4;
    if (nested_scope(3) != 17) return 5;
    return 0;
}
