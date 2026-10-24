// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <memory>
#include <type_traits>

static int seen;

struct Del {
  explicit Del(unsigned size) : size(size) {}
  void operator()(void* p) const {
    seen = (int)size;
    ::operator delete(p);
  }
  unsigned size;
};

template <class Sig>
struct Box {
  template <class T>
  int hold() {
    if constexpr (std::is_trivially_destructible<T>::value) {
      std::unique_ptr<void, Del> p(::operator new(sizeof(T)), Del(sizeof(T)));
      p.reset();
      return seen == (int)sizeof(T) ? 0 : 1;
    }
    return 2;
  }
};

struct S {
  int n;
};

int main() {
  return Box<void()>().hold<S>();
}
