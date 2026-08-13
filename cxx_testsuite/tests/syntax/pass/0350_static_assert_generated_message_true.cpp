// RUN: -std=c++26

struct unused_message {};

// A successful static assertion has no effect, so its message object's data()
// and size() are not evaluated.
static_assert(true, unused_message{});

template <bool condition>
void check() {
  static_assert(condition, unused_message{});
}

int main() {
  check<true>();
  return 0;
}
