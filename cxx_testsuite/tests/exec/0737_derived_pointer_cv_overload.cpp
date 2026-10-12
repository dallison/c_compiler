// RUN: -std=c++17
// EXPECT_EXIT: 0

struct Base {
  virtual ~Base() = default;
};

struct Middle : Base {};
struct Derived : Middle {};

template <class T>
const T* downcast(const Base* value) {
  return static_cast<const T*>(value);
}

template <class T>
T* downcast(Base* value) {
  return static_cast<T*>(value);
}

int main() {
  Derived value;
  Middle* middle = &value;
  Derived* result = downcast<Derived>(middle);
  return result == &value ? 0 : 1;
}
