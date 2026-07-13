// RUN: -std=c++20
template <typename T>
concept ExactBool = (sizeof(T) > 1);

template <typename T>
concept NestedSfinae = requires {
  requires ExactBool<T>;
  { sizeof(T) };
};

static_assert(ExactBool<int>);
static_assert(!ExactBool<char>);
static_assert(NestedSfinae<int>);
static_assert(!NestedSfinae<char>);

int main(void) {
  return 0;
}
