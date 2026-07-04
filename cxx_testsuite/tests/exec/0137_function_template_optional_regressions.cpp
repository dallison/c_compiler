// RUN: -std=c++20
// EXPECT_EXIT: 0

struct none_t {
};

template <class T>
struct MiniOptional {
  T value;

  explicit MiniOptional(T v) : value(v) {
  }

  bool has_value() const {
    return true;
  }
};

template <class T>
bool operator==(const MiniOptional<T>& value, none_t) {
  return !value.has_value();
}

template <class T>
bool operator==(none_t, const MiniOptional<T>& value) {
  return !value.has_value();
}

template <class T>
bool operator==(const MiniOptional<T>& left, const T& right) {
  return left.value == right;
}

template <class T>
bool operator==(const T& left, const MiniOptional<T>& right) {
  return right == left;
}

template <class T>
struct Slot {
  T value;

  template <class U>
  explicit Slot(U&& v) : value(static_cast<U&&>(v)) {
  }
};

template <class T, class U>
Slot<T> make_slot(U&& value) {
  return Slot<T>(static_cast<U&&>(value));
}

int main(void) {
  MiniOptional<int> value(7);
  if (!(value == 7) || !(7 == value)) {
    return 1;
  }

  Slot<int> direct(3);
  Slot<int> made = make_slot<int>(5);
  if (direct.value != 3 || made.value != 5) {
    return 2;
  }

  return 0;
}
