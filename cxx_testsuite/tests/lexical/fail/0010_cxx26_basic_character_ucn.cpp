// RUN: -std=c++26
// EXPECT: universal character name cannot name a basic character

#define STRINGIZE_IMPL(value) #value
#define STRINGIZE(value) STRINGIZE_IMPL(value)

const char* grave = STRINGIZE(\u0060);
