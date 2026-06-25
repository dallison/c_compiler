// RUN: -std=c++17
namespace adl_one {
struct Box {
  int value;
};

int value(Box box) {
  return box.value + 1;
}
}

namespace adl_two {
struct Left {
  int value;
};
}

namespace adl_three {
struct Right {
  int value;
};

int combine(adl_two::Left left, Right right) {
  return left.value + right.value;
}
}

namespace adl_enum {
enum class Color {
  red
};

int paint(Color color) {
  return 7;
}
}

namespace adl_parent {
namespace child {
struct Nested {
  int value;
};

int nested(Nested value) {
  return value.value + 3;
}
}
}

int value(int value) {
  return value;
}

int main(void) {
  adl_one::Box box = {3};
  adl_two::Left left = {4};
  adl_three::Right right = {5};
  adl_parent::child::Nested nested_box = {6};
  int selected_namespace_function = value(box);
  int selected_mixed_namespace_function = combine(left, right);
  int selected_enum_namespace_function = paint(adl_enum::Color::red);
  int selected_nested_namespace_function = nested(nested_box);
  int selected_ordinary_function = value(10);
  return selected_namespace_function + selected_mixed_namespace_function +
         selected_enum_namespace_function + selected_nested_namespace_function +
         selected_ordinary_function;
}
