#include <exception>
#include <cstdlib>

namespace std {

static terminate_handler current_terminate;

terminate_handler set_terminate(terminate_handler handler) noexcept {
  terminate_handler previous = current_terminate;
  current_terminate = handler;
  return previous;
}

terminate_handler get_terminate() noexcept {
  return current_terminate;
}

[[noreturn]] void terminate() noexcept {
  if (current_terminate) {
    current_terminate();
  }
  abort();
}

}  // namespace std

extern "C" void __davecc_terminate(void) {
  std::terminate();
}
