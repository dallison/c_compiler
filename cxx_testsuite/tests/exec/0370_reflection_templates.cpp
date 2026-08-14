// RUN: -std=c++26
// EXPECT_EXIT: 0

#include <meta>

using std::meta::info;

consteval bool same_reflection(info left, info right) {
  return left == right;
}

template <info reflection>
struct reflected_entity {
  static constexpr info value = reflection;
  static consteval bool matches(info other) {
    return reflection == other;
  }
};

template <info... reflections>
consteval int reflection_count() {
  return sizeof...(reflections);
}

template <class T>
consteval bool reflects_template_type() {
  return ^^T == ^^T;
}

static_assert(same_reflection(^^int, ^^int), "reflection parameter");
static_assert(reflected_entity<^^int>::matches(^^int), "class NTTP");
static_assert(reflection_count<^^int, ^^long, ^^char>() == 3, "pack NTTP");
static_assert(reflects_template_type<unsigned long>(), "dependent reflection");

int main() {
  return 0;
}
