#include <string>
#include <string_view>
#include <type_traits>

template <class Signature>
struct FunctionRef;

template <class Result>
struct FunctionRef<Result()> {
  template <class Function,
            class Actual = std::invoke_result_t<const Function&>,
            class = std::enable_if_t<std::is_convertible_v<Actual, Result>>>
  FunctionRef(const Function&) {}
};

int select(FunctionRef<std::string()>) { return 1; }
int select(std::string_view) { return 2; }

int main() {
  static_assert(!std::is_invocable_v<const std::string&>);
  return select(std::string("text")) != 2;
}
