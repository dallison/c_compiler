// RUN: -std=c++20
// EXPECT-ASM: _ZW11beta_mangleE6secretv
export module beta_mangle;

int secret() { return 2; }
