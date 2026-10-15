// Each instantiation of a function template has its own local classes, and
// local classes of different functions are different types even when they
// share a name.  absl's FlagOps<T> allocates its value storage through
// std::allocator<AlignedSpace>, where AlignedSpace depends on T only through
// sizeof(T); sharing one AlignedSpace between FlagOps<bool> and
// FlagOps<std::string> under-allocated the string flags.
#include <memory>

template <typename U>
unsigned long SizeOf() {
  return sizeof(U);
}

template <typename T>
unsigned long ThroughArrayBound() {
  struct AlignedSpace {
    alignas(T) unsigned char buf[sizeof(T)];
  };
  return SizeOf<AlignedSpace>();
}

template <typename T>
unsigned long ThroughMember() {
  struct Holder {
    T value;
  };
  return SizeOf<Holder>();
}

template <typename T>
unsigned long Independent() {
  struct Tag {
    int x;
  };
  return SizeOf<Tag>();
}

template <typename T>
unsigned long DirectSizeof() {
  struct Holder {
    T value;
    char extra;
  };
  static_assert(sizeof(Holder) == 2 * sizeof(T), "layout of this instantiation");
  return sizeof(Holder) + sizeof(Holder[2]) + alignof(Holder);
}

template <typename T>
unsigned long Allocated() {
  struct AlignedSpace {
    alignas(T) unsigned char buf[sizeof(T)];
  };
  std::allocator<AlignedSpace> alloc;
  AlignedSpace* space = std::allocator_traits<std::allocator<AlignedSpace>>::allocate(alloc, 1);
  std::allocator_traits<std::allocator<AlignedSpace>>::deallocate(alloc, space, 1);
  return sizeof(*space);
}

unsigned long First() {
  struct S {
    char c;
  };
  return SizeOf<S>();
}

unsigned long Second() {
  struct S {
    double d;
  };
  return SizeOf<S>();
}

struct Big {
  char data[40];
};

template <typename T>
const void* Identity() {
  static int marker;
  return &marker;
}

template <typename T>
const void* LocalIdentity() {
  struct Key {};
  return Identity<Key>();
}

int main() {
  if (ThroughArrayBound<char>() != 1) return 1;
  if (ThroughArrayBound<Big>() != 40) return 2;
  if (ThroughArrayBound<double>() != 8) return 3;
  if (ThroughMember<char>() != 1 || ThroughMember<double>() != 8) return 4;
  if (Independent<char>() != sizeof(int) || Independent<double>() != sizeof(int)) return 5;
  if (DirectSizeof<short>() != 4 + 8 + 2) return 6;
  if (DirectSizeof<double>() != 16 + 32 + 8) return 7;
  if (Allocated<char>() != 1 || Allocated<Big>() != 40) return 8;
  if (First() != 1 || Second() != 8) return 9;
  if (LocalIdentity<char>() == LocalIdentity<int>()) return 10;
  if (LocalIdentity<char>() != LocalIdentity<char>()) return 11;
  return 0;
}
