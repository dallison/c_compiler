// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>

struct NonCopyAssignable {
  NonCopyAssignable() = default;
  NonCopyAssignable& operator=(const NonCopyAssignable&) = delete;
};

struct Factory {
  template <typename T, typename... Args>
  static T* Create(Args&&... args) {
    return new T(static_cast<Args&&>(args)...);
  }
};

class Metadata {
 public:
  template <typename T>
  T* mutable_value() {
    static_assert(!std::is_copy_assignable<Container<T>>::value);
    Container<T>* container = Factory::Create<Container<T>>();
    return &container->value;
  }

 private:
  struct ContainerBase {};

  template <typename T>
  struct Container : public ContainerBase {
    T value;
  };
};

int main() {
  Metadata metadata;
  metadata.mutable_value<NonCopyAssignable>();
  return 0;
}
