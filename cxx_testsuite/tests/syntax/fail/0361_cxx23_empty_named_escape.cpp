// RUN: -std=c++23
// EXPECT: Empty named universal character escape

const char* value = "\N{}";
