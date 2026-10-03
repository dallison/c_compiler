// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <type_traits>
#include <utility>

template <class R>
using begin_t = decltype(std::declval<R&>().begin());

template <class I>
using ref_t = decltype(*std::declval<I&>());

template <class R>
using range_ref_t = ref_t<begin_t<R>>;

struct Values {
  int data[3];
  int* begin() { return data; }
};

template <class T>
struct Sink {
  T total = T();

  template <class R>
  static ref_t<begin_t<R>> first(R& range) {
    return *range.begin();
  }

  template <class R>
    requires std::is_convertible_v<range_ref_t<R>, T>
  void add_first(R& range) {
    total += *range.begin();
  }
};

static_assert(std::is_same_v<range_ref_t<Values>, int&>);

int main() {
  Values values{{4, 5, 6}};
  int& first = Sink<long>::first(values);
  first = 7;
  if (values.data[0] != 7) {
    return 1;
  }
  Sink<long> sink;
  sink.add_first(values);
  return sink.total == 7 ? 0 : 2;
}
