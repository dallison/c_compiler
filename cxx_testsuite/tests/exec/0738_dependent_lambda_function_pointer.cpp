// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <new>
#include <string>

struct ErasedCallable {
  template <class F>
  explicit ErasedCallable(F function) {
    new (storage) F(function);
    invoke = [](const void* object) {
      return (*reinterpret_cast<const F*>(object))();
    };
  }

  alignas(void*) char storage[sizeof(void*)];
  std::string (*invoke)(const void*);
};

int main() {
  int value = 7;
  ErasedCallable callable(
      [&value]() { return value == 7 ? std::string("ok") : std::string(); });
  return callable.invoke(callable.storage) == "ok" ? 0 : 1;
}
