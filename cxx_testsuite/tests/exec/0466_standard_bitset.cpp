// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <bitset>
#if !defined(__6502__) && !defined(__wasm32__)
#include <sstream>
#endif
#include <stdexcept>
#include <string>

int main() {
  std::bitset<8> bits;
  if (!bits.none() || bits.any() || bits.count() != 0 || bits.size() != 8)
    return 1;

  bits.set(1).set(3);
  if (!bits.test(1) || !bits[3] || bits.count() != 2) return 2;
  bits[2] = bits[1];
  bits[1].flip();
  if (bits[1] || !bits[2] || !~bits[1]) return 3;

  std::bitset<8> mask(0x0f);
  if ((bits & mask).to_ulong() != 0x0c) return 4;
  if ((bits | std::bitset<8>(0x80)).to_ulong() != 0x8c) return 5;
  if ((bits ^ mask).to_ulong() != 0x03) return 6;
  if ((~std::bitset<8>(0x0f)).to_ulong() != 0xf0) return 7;

  std::bitset<8> shifts(3);
  if ((shifts << 2).to_ulong() != 12) return 8;
  if ((std::bitset<8>(0xc0) >> 3).to_ulong() != 0x18) return 9;
  shifts <<= 8;
  if (shifts.any()) return 10;

  std::bitset<65> wide;
  wide.set(64).set(1);
  if (!wide[64] || wide.count() != 2) return 11;
  wide >>= 1;
  if (!wide[63] || !wide[0] || wide.count() != 2) return 12;
  wide <<= 1;
  if (!wide[64] || !wide[1] || wide.count() != 2) return 13;

  std::bitset<8> from_string(std::string("10100101"));
  if (from_string.to_ulong() != 0xa5) return 14;
  if (from_string.to_string() != "10100101") return 15;
  std::bitset<4> substring(std::string("xx1101yy"), 2, 4);
  if (substring.to_ulong() != 13) return 16;

#if !defined(__6502__) && !defined(__wasm32__)
  std::stringstream output;
  output << from_string;
  if (output.str() != "10100101") return 17;
  std::stringstream input("011010 trailing");
  std::bitset<6> streamed;
  input >> streamed;
  if (streamed.to_ulong() != 26) return 18;
#endif

  std::bitset<0> empty;
  if (empty.size() != 0 || !empty.all() || empty.any() || !empty.none())
    return 19;

  std::hash<std::bitset<8>> hasher;
  if (hasher(from_string) != hasher(from_string)) return 20;
  if (hasher(from_string) == hasher(std::bitset<8>(0))) return 21;

#ifdef __cpp_exceptions
  bool caught = false;
  try {
    bits.test(8);
  } catch (const std::out_of_range&) {
    caught = true;
  }
  if (!caught) return 22;

  caught = false;
  try {
    std::bitset<8> invalid(std::string("10x1"));
    (void)invalid;
  } catch (const std::invalid_argument&) {
    caught = true;
  }
  if (!caught) return 23;

  caught = false;
  try {
    std::bitset<65> overflow;
    overflow.set(64);
    (void)overflow.to_ullong();
  } catch (const std::overflow_error&) {
    caught = true;
  }
  if (!caught) return 24;
#endif

  return 0;
}
