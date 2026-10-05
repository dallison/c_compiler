// RUN: -std=c++23
// EXPECT_EXIT: 0

// A chrono specification the argument cannot satisfy throws format_error from
// inside the formatter; unwinding through the formatting machinery and the
// discarded result must leave the heap intact for later formatting.
#include <chrono>
#include <format>
#include <string>

int main() {
  for (int i = 0; i < 3; ++i) {
    bool rejected = false;
    try {
      (void)std::format("{:%Z}", std::chrono::seconds(1));
    } catch (const std::format_error&) {
      rejected = true;
    }
    if (!rejected) return 1 + i;
  }
  std::string s = std::format("{} {}", 42, std::string(40, 'x'));
  return s.size() == 43 ? 0 : 10;
}
