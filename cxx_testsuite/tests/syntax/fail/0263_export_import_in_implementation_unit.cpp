// RUN: -std=c++20
// EXPECT: Export declarations are not allowed in module implementation units
module syntax_export_import_impl;

export import syntax_export_import_impl;
