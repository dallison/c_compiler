// RUN: -std=c++23
// EXPECT: Empty delimited escape sequence

const char* value = "\x{}";
