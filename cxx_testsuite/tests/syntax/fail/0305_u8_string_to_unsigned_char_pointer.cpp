// RUN: -std=c++20
// EXPECT: Illegal pointer conversion

const unsigned char* text = u8"not unsigned char";
