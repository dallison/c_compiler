// RUN: -std=c++20

template <typename T, typename U>
struct Pair {
  T first;
  U second;
};

template <typename T>
struct Holder {
  T value;
  Holder(T initial) : value(initial) {
  }
};

template <typename T>
struct Wrapped {
  T value;
};

template <typename T>
Wrapped(T) -> Wrapped<T>;

int main(void) {
  Pair aggregate{1, 'a'};
  Holder constructed(7);
  Wrapped guided(3);
  (void)aggregate;
  (void)constructed;
  (void)guided;
  return 0;
}
