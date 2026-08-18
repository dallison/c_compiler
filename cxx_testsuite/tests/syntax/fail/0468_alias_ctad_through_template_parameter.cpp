// RUN: -std=c++20

template <typename T>
struct Value {
  explicit Value(T) {}
};

template <typename T, template <typename> typename Template = Value>
using Alias = Template<T>;

template <typename T>
using NestedAlias = Alias<T>;

void invalid_alias_deduction() {
  NestedAlias first(42);
  Alias second(42);
}
