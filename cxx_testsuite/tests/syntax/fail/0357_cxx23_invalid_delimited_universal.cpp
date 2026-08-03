// RUN: -std=c++23
// EXPECT: Invalid universal-character value

const char32_t* value = U"\u{d800}";
