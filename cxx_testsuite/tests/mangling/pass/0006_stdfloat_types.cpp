// RUN: -std=c++23
// EXPECT-ASM: _Z5fixedDF32_
// EXPECT-ASM: _Z5fixedDF64_

void fixed(decltype(0.0f32) value) {}
void fixed(decltype(0.0f64) value) {}

int main() { return 0; }
