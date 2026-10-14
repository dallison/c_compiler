// Block-scope thread_local variables of inline functions get storage and a
// guard even when the function's definition is emitted after its first use
// (absl container_internal::NextSeed).
#include <stdint.h>

inline uint16_t NextSeed() {
  thread_local uint16_t seed =
      static_cast<uint16_t>(reinterpret_cast<uintptr_t>(&seed));
  seed += uint16_t{0xad53};
  return seed;
}

inline int Counter() {
  static int calls = static_cast<int>(reinterpret_cast<uintptr_t>(&calls) & 0);
  return ++calls;
}

inline int A() {
  thread_local int a = 5;
  return ++a;
}

inline int B() {
  static thread_local int b = 5;
  return ++b;
}

int main() {
  uint16_t first = NextSeed();
  uint16_t second = NextSeed();
  if (static_cast<uint16_t>(second - first) != 0xad53) return 1;
  if (Counter() != 1 || Counter() != 2) return 2;
  if (A() + B() != 12) return 3;
  if (A() != 7) return 4;
  return 0;
}
