// RUN: -std=c++23
// EXPECT: Unknown Unicode character name

const char* value = "\N{NOT A UNICODE CHARACTER NAME}";
