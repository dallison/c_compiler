// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <functional>
#include <string>
#include <type_traits>

template <class Signature>
class FunctionRef;

template <class R, class... Args>
class FunctionRef<R(Args...)> {
  template <class F, class FR = std::invoke_result_t<F, Args&&...>>
  using EnableIfCompatible =
      typename std::enable_if<std::is_void<R>::value ||
                              std::is_convertible<FR, R>::value>::type;

 public:
  template <class F, class = EnableIfCompatible<const F&>>
  FunctionRef(const F&) {}
};

void Accept(FunctionRef<std::string()>) {}

int Select(FunctionRef<std::string()>) { return 1; }
int Select(...) { return 2; }

int main() {
  const char* text = "error";
  auto first = [text] { return text; };
  static_assert(
      std::is_same<std::invoke_result_t<const decltype(first)&>,
                   const char*>::value);
  static_assert(std::is_convertible<const char*, std::string>::value);
  FunctionRef<std::string()> direct(first);
  Accept(first);
  Accept([] { return std::string("message"); });
  if (Select(first) != 1) {
    return 1;
  }
  return Select([] { return 17; }) == 2 ? 0 : 2;
}
