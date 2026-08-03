// RUN: -std=c++23

consteval int identity(int value) {
  return value;
}

constexpr char identity(char value) {
  return value;
}

template <typename T>
constexpr int twice(T value) {
  if constexpr (sizeof(T) == sizeof(int)) {
    return value + identity(value);
  } else {
    return value + value;
  }
}

constexpr int plus_constant(int value) {
  return value + identity(42);
}

struct positive_function {
  consteval bool operator()(int value) const {
    return value > 0;
  }
};

template <typename Function>
constexpr bool invoke(int value, Function function) {
  return function(value);
}

template <typename T>
constexpr int guarded(T value) {
  if consteval {
    return identity(value);
  } else {
    return value;
  }
}

auto non_immediate_specialization = &twice<char>;
auto constant_call_function = &plus_constant;
auto guarded_specialization = &guarded<int>;

static_assert(twice(3) == 6);
static_assert([](int value) { return identity(value); }(3) == 3);
static_assert(invoke(3, positive_function{}));

int main() {
  return 0;
}
