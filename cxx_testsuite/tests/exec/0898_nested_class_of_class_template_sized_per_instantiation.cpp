// A class nested in a class template is laid out for each instantiation, so
// `sizeof` and `new` in the template's member functions use that layout
// (gtest's `ThreadLocal<T>::ValueHolder`), not the pattern's.
#include <stddef.h>
#include <stdlib.h>
#include <new>

static size_t last_size = 0;
void* operator new(size_t n) {
  last_size = n;
  return malloc(n ? n : 1);
}
void* operator new[](size_t n) {
  last_size = n;
  return malloc(n ? n : 1);
}
void operator delete(void* p) noexcept { free(p); }
void operator delete[](void* p) noexcept { free(p); }

struct Base {
  virtual ~Base() = default;
};
struct Big {
  long x[3];
};

template <typename T>
struct Outer {
  struct Holder : Base {
    T value;
  };
  struct Plain {
    T value;
    ~Plain() {}
  };
  struct Fixed {
    int a, b;
  };
  T t;

  static size_t HolderSize() { return sizeof(Holder); }
  static size_t PlainSize() { return sizeof(Plain); }
  static size_t FixedSize() { return sizeof(Fixed); }
  static size_t SelfSize() { return sizeof(Outer); }
  static size_t ArraySize() { return sizeof(Plain[3]); }
  static size_t PlainAlign() { return alignof(Plain); }
  static size_t NewHolder() {
    Base* b = new Holder();
    size_t size = last_size;
    delete b;
    return size;
  }
  static size_t NewPlainArray() {
    delete[] new Plain[3];
    return last_size;
  }
};

int main() {
  if (Outer<Big>::HolderSize() != sizeof(Outer<Big>::Holder)) return 1;
  if (Outer<Big>::PlainSize() != sizeof(Big)) return 2;
  if (Outer<Big>::FixedSize() != 2 * sizeof(int)) return 3;
  if (Outer<Big>::SelfSize() != sizeof(Big)) return 4;
  if (Outer<Big>::ArraySize() != 3 * sizeof(Big)) return 5;
  if (Outer<char>::PlainAlign() != 1 || Outer<char>::PlainSize() != 1) return 6;
  if (Outer<Big>::NewHolder() != sizeof(Outer<Big>::Holder)) return 7;
  if (Outer<void*>::NewHolder() != sizeof(Outer<void*>::Holder)) return 8;
  // The array cookie is one or two words, depending on the ABI.
  size_t array_size = Outer<Big>::NewPlainArray();
  if (array_size < 3 * sizeof(Big) + sizeof(size_t) ||
      array_size > 3 * sizeof(Big) + 2 * sizeof(size_t)) {
    return 9;
  }
  return 0;
}
