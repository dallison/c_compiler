// RUN: -std=c++20
// EXPECT_EXIT: 0

template <typename T>
concept Incrementable = requires(T value) {
  ++value;
};

template <typename T>
concept Dereferenceable = requires(T value) {
  *value;
};

template <typename T>
concept Iterator = Incrementable<T> && Dereferenceable<T>;

template <typename T>
concept Never = false;

template <Incrementable T>
constexpr int refined(T) {
  return 1;
}

template <Iterator T>
constexpr int refined(T) {
  return 2;
}

template <typename T>
  requires(Incrementable<T> || Never<T>)
constexpr int disjunction(T) {
  return 3;
}

template <Incrementable T>
constexpr int disjunction(T) {
  return 4;
}

int main() {
  int values[2] = {1, 2};
  int* pointer = values;
  if (refined(pointer) != 2) {
    return 1;
  }
  if (disjunction(pointer) != 4) {
    return 2;
  }
  return 0;
}
