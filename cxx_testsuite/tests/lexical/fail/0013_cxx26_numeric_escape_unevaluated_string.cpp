// RUN: -std=c++26
// EXPECT: unevaluated string in attribute argument cannot contain a numeric escape sequence

[[nodiscard("\x41")]]
int result();
