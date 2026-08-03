// RUN: -std=c++23
// EXPECT: Invalid character in named universal character escape

const char* value = "\N{Latin Capital Letter A}";
