// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>
#include <cstddef>

class Arg {
  static const size_t kInlinedSpace = 8;
  union Data {
    const void* ptr;
    char buf[kInlinedSpace];
  };

  template <typename T>
  struct store_by_value
      : std::integral_constant<bool, (sizeof(T) <= kInlinedSpace) &&
                                         std::is_integral<T>::value> {};

  enum StoragePolicy { ByPointer, ByValue };
  template <typename T>
  struct storage_policy
      : std::integral_constant<StoragePolicy,
                               (store_by_value<T>::value ? ByValue
                                                         : ByPointer)> {};

 public:
  static int check() {
    return storage_policy<int>::value == ByValue &&
                   storage_policy<Arg>::value == ByPointer
               ? 0
               : 1;
  }
};

int main() { return Arg::check(); }
