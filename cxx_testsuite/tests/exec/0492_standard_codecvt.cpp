// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <codecvt>
#include <cwchar>

int main() {
  std::mbstate_t state = {};

  std::codecvt_utf8<char32_t> utf8;
  const char32_t input[] = {U'A', 0x20ac, 0x1f600};
  const char32_t* input_next = nullptr;
  char encoded[16] = {};
  char* encoded_next = nullptr;
  if (utf8.out(state, input, input + 3, input_next,
               encoded, encoded + sizeof(encoded), encoded_next) !=
      std::codecvt_base::ok) {
    return 2;
  }
  if (input_next != input + 3 || encoded_next - encoded != 8) return 3;
  if (static_cast<unsigned char>(encoded[1]) != 0xe2 ||
      static_cast<unsigned char>(encoded[4]) != 0xf0) {
    return 4;
  }

  const char* encoded_read = nullptr;
  char32_t decoded[3] = {};
  char32_t* decoded_next = nullptr;
  if (utf8.in(state, encoded, encoded_next, encoded_read,
              decoded, decoded + 3, decoded_next) !=
      std::codecvt_base::ok) {
    return 5;
  }
  if (decoded_next != decoded + 3 || decoded[0] != U'A' ||
      decoded[1] != 0x20ac || decoded[2] != 0x1f600) {
    return 6;
  }

  std::codecvt_utf8_utf16<char16_t> utf8_utf16;
  const char16_t surrogate_pair[] = {0xd83d, 0xde00};
  const char16_t* pair_next = nullptr;
  char pair_bytes[4] = {};
  char* pair_bytes_next = nullptr;
  if (utf8_utf16.out(state, surrogate_pair, surrogate_pair + 2, pair_next,
                     pair_bytes, pair_bytes + 4, pair_bytes_next) !=
      std::codecvt_base::ok) {
    return 7;
  }
  if (pair_next != surrogate_pair + 2 ||
      pair_bytes_next != pair_bytes + 4) {
    return 8;
  }

  std::codecvt_utf16<char32_t, 0x10ffff, std::little_endian> utf16;
  const char32_t omega[] = {0x03a9};
  const char32_t* omega_next = nullptr;
  char utf16_bytes[2] = {};
  char* utf16_next = nullptr;
  if (utf16.out(state, omega, omega + 1, omega_next,
                utf16_bytes, utf16_bytes + 2, utf16_next) !=
      std::codecvt_base::ok) {
    return 9;
  }
  if (static_cast<unsigned char>(utf16_bytes[0]) != 0xa9 ||
      static_cast<unsigned char>(utf16_bytes[1]) != 0x03) {
    return 10;
  }
  return 0;
}
