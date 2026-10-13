struct Base {
  virtual ~Base() = default;
};

template <class T>
struct Memo : Base {
  T value;
};

template <class To, class From>
To down(From& from) {
  return static_cast<To>(from);
}

struct Value {
  int number;
};

struct MakeValue {
  Value operator()() const { return {}; }
};

template <class Factory>
const auto& get(Base& base, Factory factory) {
  using Result = decltype(factory());
  if (factory().number) {
    return down<const Memo<Result>&>(base).value;
  }
  Result other{};
  return down<const Memo<Result>&>(base).value;
}

int main() {
  Memo<Value> memo;
  memo.value.number = 7;
  return get(memo, MakeValue{}).number != 7;
}
