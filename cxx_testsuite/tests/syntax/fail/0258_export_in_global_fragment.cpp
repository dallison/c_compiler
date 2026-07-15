// RUN: -std=c++20
// EXPECT: Export declarations are not allowed in the global module fragment
module;
export int global_exported();
export module syntax_gmf_export;
