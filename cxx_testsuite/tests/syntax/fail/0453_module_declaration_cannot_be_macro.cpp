// RUN: -std=c++20
// EXPECT: module declarations cannot be produced by macro expansion

module;
#define DECLARE_MODULE export module macro_generated;
DECLARE_MODULE
