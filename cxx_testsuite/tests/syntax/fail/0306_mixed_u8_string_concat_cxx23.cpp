// RUN: -std=c++23
// EXPECT: Cannot concatenate string literals with different encoding prefixes

const char8_t* text = u8"UTF-8" "ordinary";
