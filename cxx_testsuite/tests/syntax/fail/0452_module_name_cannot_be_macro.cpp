// RUN: -std=c++20
// EXPECT: object-like macro 'PART' cannot be used in a module declaration

module;
#define PART detail
export module example:PART;
