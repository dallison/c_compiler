// RUN: -std=c++23
// EXPECT: Missing } in named universal character escape

const char* value = "\N{LATIN CAPITAL LETTER A";
