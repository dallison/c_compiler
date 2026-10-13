struct Base {
  virtual ~Base() = default;
};

struct Derived : Base {};

template <class T>
const T* down(const Base*) {
  return T::missing();
}

template <>
const Derived* down(const Base* value) {
  return static_cast<const Derived*>(value);
}

int main() {
  Derived value;
  const Base* base = &value;
  return down<Derived>(base) != &value;
}
