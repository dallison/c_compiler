// RUN: -std=c++20
// EXPECT: Export declarations are not allowed in module implementation units
module syntax_impl_export;

export int exported_from_impl();
