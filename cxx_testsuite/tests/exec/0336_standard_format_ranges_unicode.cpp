// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <format>
#include <chrono>
#include <locale>
#include <map>
#include <string>
#include <vector>

int main() {
  using namespace std::chrono_literals;
  std::vector<int> values;
  values.push_back(1);
  values.push_back(2);
  values.push_back(3);
  if (std::format("{}", values) != "[1, 2, 3]") {
    return 1;
  }

  std::map<std::string, int> mapping;
  mapping["one"] = 1;
  mapping["two"] = 2;
  std::string mapped = std::format("{}", mapping);
  if (mapped.find("one") == std::string::npos ||
      mapped.find("two") == std::string::npos ||
      mapped.front() != '{' || mapped.back() != '}') {
    return 2;
  }

  if (std::format("{:?}", std::string("a\nb")) != "\"a\\nb\"") {
    return 3;
  }
  if (std::format("{:^4}", std::string("\xc3\xa9")) !=
      " \xc3\xa9  ") {
    return 4;
  }
  if (std::format("{:.1}", std::string("\xc3\xa9x")) !=
      "\xc3\xa9") {
    return 5;
  }
  const std::locale classic = std::locale::classic();
  if (std::format(classic, "{:L}", 1234) != "1234") {
    return 6;
  }
  std::wstring wide = std::format(L"{} {}", 42, L"wide");
  if (wide != L"42 wide") {
    return 7;
  }

  const std::chrono::year_month_day leap_day = 2024y / std::chrono::February /
                                                29d;
  if (std::format("{:%F %a}", leap_day) != "2024-02-29 Thu") {
    return 8;
  }

  return 0;
}
