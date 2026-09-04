// RUN: -std=c++26
// EXPECT_EXIT: 0

template <class T>
concept object = requires(T value) {
  value.member;
};

template <class Scalar, object X, class Tag>
constexpr void update(Scalar scalar, X& value, Tag tag);

template <class Scalar, object X, class Tag>
constexpr void update(Scalar scalar, X& value, Tag) {
  value.member += scalar;
}

struct value_type {
  int member;
};

int main() {
  value_type value{1};
  update(2, value, 0);
  return value.member == 3 ? 0 : 1;
}
