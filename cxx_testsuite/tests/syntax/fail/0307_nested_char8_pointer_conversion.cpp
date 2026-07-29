// RUN: -std=c++20
// EXPECT: Illegal pointer conversion

const char8_t** source;
const char** destination = source;
