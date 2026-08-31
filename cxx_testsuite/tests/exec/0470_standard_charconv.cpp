// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <charconv>
#include <cstdint>
#include <limits>

template <class T>
constexpr int dependent_integer_buffer_size() {
  using U = std::make_unsigned_t<T>;
  char buffer[std::numeric_limits<U>::digits + 1] = {};
  return sizeof(buffer);
}

static_assert(dependent_integer_buffer_size<int>() ==
              std::numeric_limits<unsigned int>::digits + 1);

template <class T>
bool same_text(const char* text, T value, int base = 10) {
  char buffer[80];
  std::to_chars_result result;
  if constexpr (std::is_floating_point_v<T>) {
    result = std::to_chars(buffer, buffer + sizeof(buffer), value);
  } else {
    result = std::to_chars(buffer, buffer + sizeof(buffer), value, base);
  }
  if (result.ec != std::errc()) {
    return false;
  }
  const char* current = buffer;
  while (*text != '\0' && current != result.ptr) {
    if (*current++ != *text++) {
      return false;
    }
  }
  return *text == '\0' && current == result.ptr;
}

constexpr bool constexpr_output() {
  char buffer[16] = {};
  auto output = std::to_chars(buffer, buffer + sizeof(buffer), -255, 16);
  return output.ec == std::errc() && output.ptr == buffer + 3 &&
         buffer[0] == '-' && buffer[1] == 'f' && buffer[2] == 'f';
}

constexpr bool constexpr_input() {
  const char buffer[] = "-ff";
  int value = 0;
  auto input = std::from_chars(buffer, buffer + 3, value, 16);
  return input.ec == std::errc() && input.ptr == buffer + 3 && value == -255;
}

static_assert(__cpp_lib_to_chars >= 201611L);
static_assert(__cpp_lib_constexpr_charconv >= 202207L);
static_assert(constexpr_output());
static_assert(constexpr_input());

int main() {
  if (!same_text("0", 0)) return 10;
  if (!same_text("-42", -42)) return 11;
  if (!same_text("101010", 42, 2)) return 12;
  if (!same_text("2a", 42, 16)) return 13;
  if (!same_text("z", 35, 36)) return 14;
  if (!same_text("-2147483648",
                 std::numeric_limits<std::int32_t>::min())) {
    return 2;
  }
  if (!same_text("18446744073709551615",
                 std::numeric_limits<unsigned long long>::max())) {
    return 2;
  }

  char small[2] = {};
  auto short_output = std::to_chars(small, small + 2, 100);
  if (short_output.ptr != small + 2 ||
      short_output.ec != std::errc::value_too_large) {
    return 3;
  }

  int value = 7;
  const char decimal[] = "-123tail";
  auto decimal_result =
      std::from_chars(decimal, decimal + sizeof(decimal) - 1, value);
  if (decimal_result.ec != std::errc() || value != -123 ||
      decimal_result.ptr != decimal + 4) {
    return 4;
  }

  unsigned int hex = 0;
  const char hexadecimal[] = "DeAd!";
  auto hex_result =
      std::from_chars(hexadecimal, hexadecimal + sizeof(hexadecimal) - 1,
                      hex, 16);
  if (hex_result.ec != std::errc() || hex != 0xdeadU ||
      hex_result.ptr != hexadecimal + 4) {
    return 5;
  }

  value = 91;
  const char invalid[] = " +1";
  auto invalid_result =
      std::from_chars(invalid, invalid + sizeof(invalid) - 1, value);
  if (invalid_result.ptr != invalid ||
      invalid_result.ec != std::errc::invalid_argument || value != 91) {
    return 6;
  }

  unsigned int unsigned_value = 17;
  const char negative[] = "-1";
  auto negative_result =
      std::from_chars(negative, negative + 2, unsigned_value);
  if (negative_result.ptr != negative ||
      negative_result.ec != std::errc::invalid_argument ||
      unsigned_value != 17) {
    return 7;
  }

  value = 23;
  const char overflow[] = "21474836480rest";
  auto overflow_result =
      std::from_chars(overflow, overflow + sizeof(overflow) - 1, value);
  if (overflow_result.ec != std::errc::result_out_of_range ||
      overflow_result.ptr != overflow + 11 || value != 23) {
    return 8;
  }

  signed char byte = 0;
  const char byte_text[] = "-128";
  auto byte_result =
      std::from_chars(byte_text, byte_text + 4, byte);
  if (byte_result.ec != std::errc() || byte != -128) {
    return 9;
  }

  if (!same_text("0", 0.0)) return 20;
  {
    char nzero[8];
    auto nzero_out = std::to_chars(nzero, nzero + sizeof(nzero), -0.0);
    if (nzero_out.ec != std::errc()) return 21;
    bool neg_zero =
        nzero_out.ptr == nzero + 2 && nzero[0] == '-' && nzero[1] == '0';
    bool pos_zero = nzero_out.ptr == nzero + 1 && nzero[0] == '0';
    if (!neg_zero && !pos_zero) return 21;
  }
  if (!same_text("1", 1.0)) return 22;
  if (!same_text("2", 2.0)) return 23;
  if (!same_text("0.5", 0.5)) return 24;
  if (!same_text("1.5", 1.5)) return 25;
  if (!same_text("inf", std::numeric_limits<double>::infinity())) return 26;
  if (!same_text("-inf", -std::numeric_limits<double>::infinity())) return 27;

  char nan_buf[8];
  auto nan_out = std::to_chars(nan_buf, nan_buf + sizeof(nan_buf),
                                 std::numeric_limits<double>::quiet_NaN());
  if (nan_out.ec != std::errc() || nan_out.ptr != nan_buf + 3 ||
      nan_buf[0] != 'n' || nan_buf[1] != 'a' || nan_buf[2] != 'n') {
    return 28;
  }

  char fixed_buf[16];
  auto fixed_out = std::to_chars(fixed_buf, fixed_buf + sizeof(fixed_buf), 1.5,
                                  std::chars_format::fixed, 2);
  if (fixed_out.ec != std::errc() || fixed_out.ptr != fixed_buf + 4 ||
      fixed_buf[0] != '1' || fixed_buf[1] != '.' || fixed_buf[2] != '5' ||
      fixed_buf[3] != '0') {
    return 29;
  }

  char hex_buf[16];
  auto hex_out = std::to_chars(hex_buf, hex_buf + sizeof(hex_buf), 1.0,
                                 std::chars_format::hex);
  if (hex_out.ec != std::errc() || hex_out.ptr != hex_buf + 4 ||
      hex_buf[0] != '1' || hex_buf[1] != 'p' || hex_buf[2] != '+' ||
      hex_buf[3] != '0') {
    return 30;
  }

  char tiny[2];
  auto tiny_out = std::to_chars(tiny, tiny + 2, 1.5);
  if (tiny_out.ptr != tiny + 2 || tiny_out.ec != std::errc::value_too_large) {
    return 31;
  }

  double parsed = 0;
  const char one_point_five[] = "1.5tail";
  auto parsed_in =
      std::from_chars(one_point_five, one_point_five + 7, parsed);
  if (parsed_in.ec != std::errc() || parsed != 1.5 ||
      parsed_in.ptr != one_point_five + 3) {
    return 32;
  }

  parsed = 0;
  const char sci[] = "1e2";
  auto sci_in = std::from_chars(sci, sci + 3, parsed);
  if (sci_in.ec != std::errc() || parsed != 100.0 || sci_in.ptr != sci + 3) {
    return 33;
  }

  parsed = 0;
  const char inf_text[] = "inf";
  auto inf_in = std::from_chars(inf_text, inf_text + 3, parsed);
  if (inf_in.ec != std::errc() ||
      parsed != std::numeric_limits<double>::infinity()) {
    return 34;
  }

  parsed = 7;
  const char bad[] = " +1";
  auto bad_in = std::from_chars(bad, bad + 3, parsed);
  if (bad_in.ptr != bad || bad_in.ec != std::errc::invalid_argument ||
      parsed != 7) {
    return 35;
  }

  parsed = 0;
  const char huge[] = "1e9999";
  auto huge_in = std::from_chars(huge, huge + 6, parsed);
  if (huge_in.ec != std::errc::result_out_of_range ||
      parsed != std::numeric_limits<double>::infinity()) {
    return 36;
  }

  float values[] = {0.1f, 2.0f, 3.14159f, 16.5f, -4.25f};
  for (float original : values) {
    char round[32];
    auto written = std::to_chars(round, round + sizeof(round), original);
    if (written.ec != std::errc()) return 37;
    float restored = 0;
    auto read = std::from_chars(round, written.ptr, restored);
    if (read.ec != std::errc() || restored != original) return 38;
  }

  double hex_value = 0;
  const char hex_text[] = "1.8p+1";
  auto hex_in = std::from_chars(hex_text, hex_text + 6, hex_value,
                                 std::chars_format::hex);
  if (hex_in.ec != std::errc() || hex_value != 3.0) return 39;

  return 0;
}
