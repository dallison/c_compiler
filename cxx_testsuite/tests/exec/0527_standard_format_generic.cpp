// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <bitset>
#include <complex>
#include <format>
#include <optional>
#include <string>
#include <tuple>
#include <variant>

int main() {
  if (std::format("{}", std::make_tuple(1, 2)) != "(1, 2)") {
    return 1;
  }
  if (std::format("{}", std::optional<int>(7)) != "7") {
    return 2;
  }
  if (std::format("{}", std::optional<int>()) != "nullopt") {
    return 3;
  }
  std::variant<int, std::string> value = 4;
  if (std::format("{}", value) != "4") {
    return 4;
  }
  if (std::format("{}", std::bitset<4>(10)) != "1010") {
    return 5;
  }
  if (std::format("{}", std::complex<double>(1, 2)).find("1") ==
      std::string::npos) {
    return 6;
  }
  return 0;
}
