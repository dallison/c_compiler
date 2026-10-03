// RUN: -std=c++20
// EXPECT_EXIT: 0

// A const reference bound to a converted scalar binds to a temporary holding
// the converted value, never to the source object or to the value itself.

template <class T>
struct Pair {
  Pair(const T& first = T(), const T& second = T())
      : first(first), second(second) {}
  T first;
  T second;
};

template <class T>
__attribute__((noinline)) T add_one(T value) {
  Pair<T> one(T(1));
  return value + one.first + one.second;
}

static long widen(const long& value) { return value; }
static double to_double(const double& value) { return value; }

int main() {
  if (add_one(2.0) != 3.0) return 1;
  if (add_one<long>(41) != 42) return 2;

  int i = 1;
  const int& same = i;
  const long& widened = i;
  i = 2;
  if (same != 2) return 3;
  if (widened != 1) return 4;

  if (widen(i) != 2) return 5;
  if (to_double(i) != 2.0) return 6;
  float f = 2.5f;
  if (to_double(f) != 2.5) return 7;
  return 0;
}
