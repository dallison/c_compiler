// RUN: -std=c++20
#include <type_traits>

template <class T>
struct RemoveExtent {
  using type = T;
};

template <class T, unsigned long N>
struct RemoveExtent<T[N]> {
  using type = T;
};

template <class T>
struct RemoveExtent<T[]> {
  using type = T;
};

static_assert(std::is_same<RemoveExtent<int[]>::type, int>::value);
static_assert(std::is_same<RemoveExtent<int[4]>::type, int>::value);

template <class T>
struct Box {
  T value;

  explicit Box(T initial) : value(initial) {
  }

  void swap(Box& other) {
    T temporary = value;
    value = other.value;
    other.value = temporary;
  }

  Box& operator=(const Box& other) {
    Box(other).swap(*this);
    return *this;
  }
};

namespace qualified {

template <class T>
struct Temporary {
  void touch() {
  }
};

}  // namespace qualified

inline void exercise_qualified_temporary() {
  qualified::Temporary<int>().touch();
}

template <class T>
struct TypeAligned {
  alignas(T) unsigned char storage[sizeof(T)];
};

template <class T>
struct alignas(T) AlignedClass {
  char value;
};

template <class T>
struct ExpressionAligned {
  alignas(alignof(T)) char value;
};

template <int N>
struct ValueAligned {
  alignas(N) char value;
};

struct Large {
  long first;
  long second;
};

static_assert(sizeof(TypeAligned<Large>) == sizeof(Large));
static_assert(alignof(TypeAligned<double>) == alignof(double));
static_assert(alignof(AlignedClass<double>) == alignof(double));
static_assert(alignof(ExpressionAligned<double>) == alignof(double));
static_assert(alignof(ValueAligned<8>) == 8);
