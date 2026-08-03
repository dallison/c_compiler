// RUN: -std=c++20
// EXPECT: Illegal escape sequence \N

const char* value = "\N{LATIN CAPITAL LETTER A}";
