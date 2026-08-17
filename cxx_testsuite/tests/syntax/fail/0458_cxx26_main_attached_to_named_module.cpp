// RUN: -std=c++26
// EXPECT: 'main' cannot be attached to a named module

export module main_attachment;

int main() {
  return 0;
}
