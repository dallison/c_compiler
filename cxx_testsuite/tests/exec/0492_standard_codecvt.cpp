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

  std::mbstate_t bom_state = {};
  std::codecvt_utf8<char32_t, 0x10ffff, std::generate_header> utf8_bom;
  const char32_t letter[] = {U'A'};
  const char32_t* letter_next = nullptr;
  char bom_bytes[8] = {};
  char* bom_next = nullptr;
  if (utf8_bom.out(bom_state, letter, letter + 1, letter_next,
                   bom_bytes, bom_bytes + 8, bom_next) !=
      std::codecvt_base::ok) {
    return 11;
  }
  if (bom_next - bom_bytes != 4 ||
      static_cast<unsigned char>(bom_bytes[0]) != 0xef ||
      static_cast<unsigned char>(bom_bytes[3]) != 'A') {
    return 12;
  }
  char extra[4] = {};
  char* extra_next = nullptr;
  if (utf8_bom.out(bom_state, letter, letter + 1, letter_next,
                   extra, extra + 4, extra_next) != std::codecvt_base::ok) {
    return 13;
  }
  if (extra_next - extra != 1 || extra[0] != 'A') {
    return 14;
  }

  std::mbstate_t consume_state = {};
  std::codecvt_utf8<char32_t, 0x10ffff, std::consume_header> utf8_consume;
  const char* bom_read = nullptr;
  char32_t decoded_letter[1] = {};
  char32_t* decoded_letter_next = nullptr;
  if (utf8_consume.in(consume_state, bom_bytes, bom_next, bom_read,
                      decoded_letter, decoded_letter + 1,
                      decoded_letter_next) != std::codecvt_base::ok ||
      decoded_letter[0] != U'A') {
    return 15;
  }

  char bad[] = {static_cast<char>(0xff)};
  const char* bad_next = nullptr;
  char32_t bad_out[1] = {};
  char32_t* bad_out_next = nullptr;
  std::mbstate_t error_state = {};
  if (utf8.in(error_state, bad, bad + 1, bad_next, bad_out, bad_out + 1,
              bad_out_next) != std::codecvt_base::error) {
    return 16;
  }

  char16_t roundtrip[2] = {};
  char16_t* roundtrip_next = nullptr;
  const char* pair_read = nullptr;
  if (utf8_utf16.in(state, pair_bytes, pair_bytes_next, pair_read,
                    roundtrip, roundtrip + 2, roundtrip_next) !=
          std::codecvt_base::ok ||
      roundtrip[0] != 0xd83d || roundtrip[1] != 0xde00) {
    return 17;
  }

  std::mbstate_t length_state = {};
  if (utf8.length(length_state, encoded, encoded_next, 2) != 4) {
    return 18;
  }

  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
  if (converter.from_bytes("A").size() != 1 || converter.converted() != 1) {
    return 19;
  }
  return 0;
}
