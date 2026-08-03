// RUN: -std=c++23
// EXPECT: Delimited escape value is not representable in literal encoding

const char8_t* value = u8"\x{100}";
