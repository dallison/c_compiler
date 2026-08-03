// RUN: -std=c++23
// EXPECT: consteval function call is not a constant expression

struct immediate_member {
  int value;

  constexpr immediate_member(int input) : value(input) {}
  consteval immediate_member(const immediate_member& other)
      : value(other.value) {}
};

struct wrapper {
  immediate_member member;

  constexpr wrapper(int input) : member(input) {}
  wrapper(const wrapper&) = default;
};

int runtime_copy(const wrapper& input) {
  wrapper copy = input;
  return copy.member.value;
}
