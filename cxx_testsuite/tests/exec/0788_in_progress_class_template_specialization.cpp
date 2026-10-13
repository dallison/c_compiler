struct Wrapper {
  template <bool Owned>
  struct Value;

  using OwnedValue = Value<true>;
};

template <bool Owned>
struct Wrapper::Value {
  Value() = default;

  template <class T>
  Value(T&&) {
    static_assert(Owned);
  }

  OwnedValue* same_kind = nullptr;

  int get() const { return 7; }
};

int main() {
  Wrapper::OwnedValue value;
  return value.get() != 7;
}
