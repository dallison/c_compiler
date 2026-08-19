// RUN: -std=c++29
// EXPECT_EXIT: 0

#include <meta>

using namespace std::meta;

enum class command : unsigned char;

constexpr auto open = enumerator_spec({.name = "open"});
constexpr auto close = enumerator_spec({.name = "close"});
constexpr auto retry =
    enumerator_spec({.name = "retry", .value = reflect_constant(7)});
constexpr auto stop = enumerator_spec({.name = "stop"});
constexpr auto hole_one = enumerator_spec({.name = "_"});
constexpr auto hole_two = enumerator_spec({.name = "_"});

consteval {
  define_enum(^^command, {open, close, retry, stop, hole_one, hole_two});
}

int dispatch(command value) {
  switch (value) {
    case command::open:
      return 11;
    case command::close:
      return 22;
    case command::retry:
      return 33;
    case command::stop:
      return 44;
    default:
      return 55;
  }
}

int main() {
  return dispatch(command::open) == 11 && dispatch(command::close) == 22 &&
                 dispatch(command::retry) == 33 &&
                 dispatch(command::stop) == 44
             ? 0
             : 1;
}
