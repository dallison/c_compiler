// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <utility>

template <typename KeyType, typename ValueType>
struct Factory {
  template <typename Key, typename... Args>
  int Create(Key&&, Args&&...) {
    return sizeof...(Args);
  }

  template <typename Key, typename... Args>
  int Emplace(Key&& key, Args&&... args) {
    return Create(std::forward<Key>(key), std::forward<Args>(args)...);
  }
};

int main() {
  Factory<int, int> factory;
  int first = factory.Emplace(1);
  int second = factory.Emplace(1, 2);
  int third = factory.Emplace(1, 2, 3);
  return first == 0 && second == 1 && third == 2 ? 0 : 1;
}
