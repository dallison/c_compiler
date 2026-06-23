template <typename T, typename U>
struct Pair {
  T first;
  U second;
};

template <typename T>
struct Holder {
  T value;
  Holder(T initial) : value(initial + 1) {
  }
};

template <typename T>
struct Wrapped {
  T value;
};

template <typename T>
Wrapped(T) -> Wrapped<T>;

int main(void) {
  Pair aggregate{3, 4};
  Holder constructed(5);
  Wrapped guided{6};
  if (aggregate.first != 3 || aggregate.second != 4) {
    return 1;
  }
  if (constructed.value != 6) {
    return 2;
  }
  if (guided.value != 6) {
    return 3;
  }
  return 0;
}
