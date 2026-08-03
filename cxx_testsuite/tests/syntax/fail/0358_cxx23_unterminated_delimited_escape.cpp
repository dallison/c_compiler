// RUN: -std=c++23
// EXPECT: Missing } in delimited escape sequence

const char* value = "\o{101";
