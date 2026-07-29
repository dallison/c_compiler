// RUN: -std=c++20
// EXPECT: Escape value is not representable in char8_t

const char8_t* text = u8"\x100";
