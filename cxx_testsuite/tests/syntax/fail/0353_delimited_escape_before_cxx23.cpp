// RUN: -std=c++20
// EXPECT: A universal-character must have 4 hex digits

const char* text = "\u{41}";
