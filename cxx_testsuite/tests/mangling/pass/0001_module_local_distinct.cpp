// RUN: -std=c++20
// EXPECT-ASM: _ZW12alpha_mangleE6secretv
export module alpha_mangle;

int secret() { return 1; }
