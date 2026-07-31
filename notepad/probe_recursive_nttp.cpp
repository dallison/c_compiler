template <unsigned N>
struct recursive : recursive<N - 1> {};

template <>
struct recursive<0> {};

recursive<2> value;

template <class T, unsigned N>
struct typed_recursive : typed_recursive<T, N - 1> {
  T item;
};

template <class T>
struct typed_recursive<T, 0> {};

typed_recursive<int, 2> typed_value;

#include <mdspan>

template <class T, std::size_t N>
struct sized_recursive : sized_recursive<T, N - 1> {
  T item;

  constexpr T& operator[](std::size_t index) {
    return index + 1 == N ? item
                          : sized_recursive<T, N - 1>::operator[](index);
  }
};

template <class T>
struct sized_recursive<T, 0> {
  constexpr T& operator[](std::size_t) {
    return *static_cast<T*>(nullptr);
  }
};

sized_recursive<int, 2> sized_value;
