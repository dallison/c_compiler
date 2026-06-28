// RUN: -std=c++20
#include <memory>
#include <type_traits>
#include <utility>

static_assert(std::is_same<std::remove_reference_t<int&>, int>::value,
              "remove_reference_t<int&>");
static_assert(std::is_same<std::remove_reference_t<int&&>, int>::value,
              "remove_reference_t<int&&>");
static_assert(std::is_lvalue_reference<int&>::value,
              "is_lvalue_reference");
static_assert(std::is_rvalue_reference<int&&>::value,
              "is_rvalue_reference");
static_assert(std::is_same<std::conditional<true, int, long>::type, int>::value,
              "conditional true");
static_assert(std::is_same<std::conditional<false, int, long>::type, long>::value,
              "conditional false");

struct ForwardedBox {
  int kind;

  ForwardedBox(int& value) : kind(value) {
  }

  ForwardedBox(int&& value) : kind(value + 10) {
  }
};

template <class T>
ForwardedBox make_forwarded_box(T&& value) {
  return ForwardedBox(std::forward<T>(value));
}

template <class... Args>
ForwardedBox make_forwarded_box_pack(Args&&... args) {
  return ForwardedBox(std::forward<Args>(args)...);
}

int main(void) {
  int value = 3;
  int& lvalue = std::forward<int&>(value);
  int rvalue = std::forward<int>(value);
  (void)lvalue;
  (void)rvalue;

  ForwardedBox lbox = make_forwarded_box(value);
  ForwardedBox rbox = make_forwarded_box(4);
  ForwardedBox packed = make_forwarded_box_pack(value);
  std::unique_ptr<ForwardedBox> made =
      std::make_unique<ForwardedBox>(value);
  int old = std::exchange(value, 9);
  std::swap(old, value);
  std::unique_ptr<ForwardedBox> moved = std::move(made);
  (void)lbox;
  (void)rbox;
  (void)packed;
  (void)moved;
  return 0;
}
