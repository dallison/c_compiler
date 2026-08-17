// RUN: -std=c++26
// EXPECT: an entity named 'main' cannot have C language linkage

extern "C" int main() {
  return 0;
}
