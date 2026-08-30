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
// One array, accessed in straight-line code right after its declaration, is as
// much as every target manages today.  Two arrays in one scope, or a base held
// across a loop or a call, still fail on aarch64: its allocator hands the same
// register to two of these bases, and cannot model a live range that spans a
// loop's back edge.  Those cases belong here once that is fixed.
//
// The check lives in a callee because the exec harness enters at main.

__attribute__((noinline)) static int single_array(int n) {
    char v[n];
    v[0] = 3;
    v[1] = 4;
    v[n - 1] = 5;
    return v[0] + v[1] + v[n - 1];
}

int main() {
    if (single_array(4) != 12) return 1;
    return 0;
}
