// RUN: -std=c++26
// EXPECT_EXIT: 0

struct Value {
  int first;
  int second;
};

struct Converted {
  int value;
  constexpr Converted(int source) : value(source) {}
};

struct ExternalPointer {
  const int* pointer;
};

struct FloatingValue {
  double value;
};

int pointed_value = 9;
constexpr Value named_value{17, 25};

template <Value object>
int sum() {
  return object.first + object.second;
}

template <Value object>
const Value* parameter_object_address() {
  return &object;
}

template <Value object>
const Value* same_parameter_object_address() {
  return &object;
}

template <Converted object>
int converted_value() {
  return object.value;
}

template <Value object = Value{20, 22}>
int defaulted_sum() {
  return object.first + object.second;
}

template <ExternalPointer object>
int read_external_pointer() {
  return *object.pointer;
}

template <FloatingValue object>
int scaled_floating_value() {
  return static_cast<int>(object.value * 2.0);
}

int main() {
  if (sum<named_value>() != 42) {
    return 1;
  }
  if (sum<Value{2, 3}>() != 5) {
    return 2;
  }
  if (parameter_object_address<Value{17, 25}>() !=
      same_parameter_object_address<Value{17, 25}>()) {
    return 3;
  }
  if (converted_value<42>() != 42) {
    return 4;
  }
  if (defaulted_sum<>() != 42) {
    return 5;
  }
  if (read_external_pointer<ExternalPointer{&pointed_value}>() != 9) {
    return 6;
  }
  if (read_external_pointer<ExternalPointer{&pointed_value}>() != 9) {
    return 7;
  }
  if (scaled_floating_value<FloatingValue{1.5}>() != 3) {
    return 8;
  }
  return 0;
}
