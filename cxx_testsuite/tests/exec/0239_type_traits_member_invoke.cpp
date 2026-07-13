// RUN: -std=c++20
#include <type_traits>

struct Box {
  int value;
  int scale(int factor) const { return value * factor; }
  void bump() { value += 1; }
};

int main() {
  Box box{6};
  int Box::* data = &Box::value;

  if ((box.*data) != 6) {
    return 1;
  }
  if ((box.*(&Box::scale))(3) != 18) {
    return 2;
  }
  int (Box::* scale_fn)(int) const = &Box::scale;
  if ((box.*scale_fn)(4) != 24) {
    return 3;
  }
  void (Box::* bump_fn)() = &Box::bump;
  (box.*bump_fn)();
  if (box.value != 7) {
    return 4;
  }
  if (!std::is_member_object_pointer<decltype(data)>::value) {
    return 5;
  }
  if (!std::is_member_function_pointer<decltype(scale_fn)>::value) {
    return 6;
  }
  return 0;
}
