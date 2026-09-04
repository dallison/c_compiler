// RUN: -std=c++26
// EXPECT_EXIT: 0

#include <debugging>

#if __cpp_lib_debugging != 202403L
#error "unexpected __cpp_lib_debugging value"
#endif

static_assert(noexcept(std::breakpoint()));
static_assert(noexcept(std::breakpoint_if_debugging()));
static_assert(noexcept(std::is_debugger_present()));

int main() {
  if (std::is_debugger_present()) {
    return 1;
  }
  std::breakpoint_if_debugging();
  return 0;
}
