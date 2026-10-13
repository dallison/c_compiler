// RUN: -std=c++17
// EXPECT_EXIT: 0

// invoke_result must keep the callable's value category so a ref-qualified
// call operator is selected exactly as the real call would select it.
#include <functional>
#include <type_traits>

struct LvalueOnly {
  int operator()(int value) & { return value + 1; }
};

struct ConstLvalueOnly {
  long operator()(int value) const& { return value + 2; }
};

struct RvalueOnly {
  char operator()(int) && { return 'r'; }
};

static_assert(std::is_same_v<std::invoke_result_t<LvalueOnly&, int>, int>);
static_assert(
    std::is_same_v<std::invoke_result_t<const ConstLvalueOnly&, int>, long>);
static_assert(std::is_same_v<std::invoke_result_t<RvalueOnly, int>, char>);
static_assert(std::is_invocable_r_v<int, LvalueOnly&, int>);
static_assert(!std::is_invocable_v<LvalueOnly, int>);
static_assert(!std::is_invocable_v<RvalueOnly&, int>);
static_assert(std::is_same_v<
              std::invoke_result_t<std::reference_wrapper<LvalueOnly>, int>,
              int>);

int main() {
  LvalueOnly target;
  std::reference_wrapper<LvalueOnly> ref(target);
  return std::invoke(ref, 4) == 5 ? 0 : 1;
}
