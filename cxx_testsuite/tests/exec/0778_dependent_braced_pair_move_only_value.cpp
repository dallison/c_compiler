#include <memory>
#include <utility>

struct Base {
  virtual ~Base() = default;
};

struct Derived : Base {};

struct Map {
  using Key = std::pair<const void*, const void*>;
  using Value = std::pair<Key, std::unique_ptr<Base>>;

  struct InsertResult {
    int first;
  };

  InsertResult insert(Value&& value) {
    return {value.second ? 1 : 0};
  }

  int end() const { return 0; }
};

template <class T>
int test() {
  Map map;
  Map::Key key{};
  auto value = std::make_unique<T>();
  auto insert_result = map.insert({key, std::move(value)});
  auto it = insert_result.first;
  return it == map.end();
}

int main() { return test<Derived>(); }
