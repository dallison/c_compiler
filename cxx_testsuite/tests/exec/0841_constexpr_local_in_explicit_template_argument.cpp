// A constexpr local used in a later local alias that names an explicit
// template argument (`m.template Get<TypeToUse>()`, as in protobuf's
// FlatAllocator) reads the instantiation's value, so the call through it is
// resolved instead of naming the uninstantiated function template.
#include <string>
#include <type_traits>

template <typename T>
const T& Ref(const T& t) {
  return t;
}

struct Slots {
  template <typename U>
  int& Get() {
    return std::is_same<U, char>::value ? chars : others;
  }
  int chars = 1;
  int others = 2;
};

Slots slots;

template <typename U>
int FreeSlot() {
  constexpr bool trivial = std::is_trivially_destructible<U>::value;
  using TypeToUse = typename std::conditional<trivial, char, U>::type;
  return Ref(slots.template Get<TypeToUse>());
}

template <typename... T>
struct Allocator {
  template <typename U>
  int Slot() {
    constexpr bool trivial = std::is_trivially_destructible<U>::value;
    using TypeToUse = typename std::conditional<trivial, char, U>::type;
    return Ref(slots_.template Get<TypeToUse>());
  }
  template <typename U>
  int LiteralSlot() {
    constexpr bool trivial = sizeof(U) == 0;
    using TypeToUse = std::conditional_t<trivial, char, U>;
    return Ref(slots_.template Get<TypeToUse>());
  }
  Slots slots_;
};

int main() {
  if (FreeSlot<int>() != 1 || FreeSlot<std::string>() != 2) {
    return 1;
  }
  Allocator<char, int> a;
  if (a.Slot<int>() != 1 || a.Slot<std::string>() != 2) {
    return 2;
  }
  return a.LiteralSlot<char>() == 1 && a.LiteralSlot<int>() == 2 ? 0 : 3;
}
