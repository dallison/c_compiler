// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <chrono>
#include <format>
#include <print>
#include <thread>

int main() {
  std::format_string<int, int, int, int, int> text("{}{}{}{}{}");
  if (text.size() != 10) {
    return 11;
  }
  int a = 1;
  int b = 2;
  int c = 3;
  int d = 4;
  int e = 5;
  std::string result = std::format("{},{},{},{},{}", a, b, c, d, e);
  if (result != "1,2,3,4,5") return 1;

  if (std::format("{}{}{}{}{}{}{}", 1, 2, 3, 4, 5, 6, 7) !=
      "1234567") {
    return 2;
  }
  int f = 6;
  auto args = std::make_format_args(a, b, c, d, e, f);
  if (std::vformat("{}{}{}{}{}{}", args) != "123456") {
    return 3;
  }
  if (std::formatted_size("{}{}{}{}{}{}", a, b, c, d, e, f) != 6) {
    return 4;
  }

  std::formatter<std::chrono::seconds, char> duration_formatter;
  std::format_parse_context duration_parse("%Q %q");
  duration_formatter.parse(duration_parse);
  if (duration_formatter.__specification_size != 5)
    return 40 + duration_formatter.__specification_size;
  if (duration_formatter.__specification_data[0] != '%')
    return duration_formatter.__specification_data[0];

  if (std::format("{}", std::chrono::milliseconds(1250)) != "1250ms") {
    return 5;
  }
  if (std::format("{:%Q %q}", std::chrono::seconds(42)) != "42 s") {
    return 6;
  }
  if (std::format("{:%T}", std::chrono::seconds(3661)) != "01:01:01") {
    return 7;
  }
  using system_time = std::chrono::system_clock::time_point;
  if (std::format("{}", system_time(std::chrono::seconds(0))) !=
      "1970-01-01 00:00:00") {
    return 8;
  }
  system_time after_epoch(std::chrono::seconds(1));
  if (std::format("{:%F %T}", after_epoch) !=
      "1970-01-01 00:00:01") {
    return 9;
  }
  system_time before_epoch(std::chrono::seconds(-1));
  if (std::format("{:%F %T}", before_epoch) !=
      "1969-12-31 23:59:59") {
    return 10;
  }
  if (std::format("{}", std::thread::id()) != "0") {
    return 12;
  }
  if (std::format("{:04}", std::thread::id()) != "0000") {
    return 13;
  }
  bool rejected_invalid_specification = false;
  try {
    (void)std::format("{:%Z}", std::chrono::seconds(1));
  } catch (const std::format_error&) {
    rejected_invalid_specification = true;
  }
  if (!rejected_invalid_specification) {
    return 14;
  }

  return 0;
}
