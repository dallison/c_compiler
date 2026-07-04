// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <utility>

struct Source {
  int value;

  explicit Source(int v) : value(v) {
  }
};

struct Sink {
  int category;
  int value;

  Sink(Source& source) : category(1), value(source.value) {
  }

  Sink(Source&& source) : category(2), value(source.value) {
    source.value = -1;
  }
};

template <class... Args>
int construct_kind(Args&&... args) {
  Sink sink(std::forward<Args>(args)...);
  return sink.category * 100 + sink.value;
}

int main(void) {
  Source lvalue(11);
  if (construct_kind(lvalue) != 111 || lvalue.value != 11) {
    return 1;
  }

  Source rvalue(22);
  if (construct_kind(std::move(rvalue)) != 222 || rvalue.value != -1) {
    return 2;
  }

  return 0;
}
