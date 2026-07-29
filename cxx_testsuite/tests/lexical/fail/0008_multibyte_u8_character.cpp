// RUN: -std=c++20
// EXPECT: UTF-8 character literal must contain exactly one code unit

char8_t value = u8'\u00a9';
