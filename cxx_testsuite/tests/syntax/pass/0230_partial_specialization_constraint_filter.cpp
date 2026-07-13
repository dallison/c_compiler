// RUN: -std=c++20

template <typename T>
concept Never = false;

template <typename T>
struct Holder {
  static constexpr int value = 1;
};

template <typename T>
  requires Never<T>
struct Holder<T*> {
  static constexpr int value = 2;
};

static_assert(Holder<int*>::value == 1);
