// RUN: -std=c++20

#include <sstream>
#include <syncstream>
#include <type_traits>
#include <utility>

#if __cpp_lib_syncbuf != 201803L
#error "__cpp_lib_syncbuf has the wrong value"
#endif

static_assert(std::is_same_v<std::syncbuf, std::basic_syncbuf<char>>);
static_assert(std::is_same_v<std::osyncstream,
                             std::basic_osyncstream<char>>);
static_assert(std::is_same_v<std::wsyncbuf,
                             std::basic_syncbuf<wchar_t>>);
static_assert(std::is_same_v<std::wosyncstream,
                             std::basic_osyncstream<wchar_t>>);

int main() {
  std::stringbuf wrapped;
  std::syncbuf first(&wrapped);
  first.set_emit_on_sync(true);
  (void)first.get_allocator();
  (void)first.get_wrapped();
  std::syncbuf second(std::move(first));
  first = std::move(second);
  swap(first, second);

  std::osyncstream output(&wrapped);
  output << 42;
  std::osyncstream moved(std::move(output));
  output = std::move(moved);
  output.emit();
  (void)output.rdbuf();
  (void)output.get_wrapped();
  return 0;
}
