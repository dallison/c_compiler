// RUN: -std=c++20
// A qualified value from a dependent class-template specialization is a
// non-type template argument even when the specialization itself is known.
enum class Kind { first, second };

template <class T, class U>
struct KindFor {
  static constexpr Kind value = Kind::first;
};

template <class T, class U, Kind K>
struct Selected {};

template <class T>
using Selection = Selected<T, T, KindFor<T, T>::value>;

Selection<int> selected;
