// RUN: -std=c++17
// EXPECT_EXIT: 0

template <class T>
struct Value {
  T value{};
};

template <template <class> class ValueT, class... Keys>
class TypeMap {
 public:
  template <class K>
  ValueT<K>& Get() {
    return static_cast<Base<K>&>(payload_).value;
  }

  template <class K>
  const ValueT<K>& Get() const {
    return static_cast<const Base<K>&>(payload_).value;
  }

 private:
  template <class K>
  struct Base {
    ValueT<K> value{};
  };

  struct Payload : Base<Keys>... {};
  Payload payload_;
};

int main() {
  TypeMap<Value> empty;
  TypeMap<Value, char, int> values;
  values.Get<char>() = Value<char>{'x'};
  values.Get<int>() = Value<int>{42};
  const auto& const_values = values;
  (void)empty;
  return const_values.Get<char>().value == 'x' &&
                 const_values.Get<int>().value == 42
             ? 0
             : 1;
}
