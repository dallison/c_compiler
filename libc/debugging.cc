#include <debugging>

namespace std {

__attribute__((weak)) bool is_debugger_present() noexcept {
  return false;
}

void breakpoint() noexcept {
  __builtin_trap();
}

void breakpoint_if_debugging() noexcept {
  if (is_debugger_present()) {
    breakpoint();
  }
}

}  // namespace std
