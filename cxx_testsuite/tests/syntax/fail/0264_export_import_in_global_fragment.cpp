// RUN: -std=c++20
// EXPECT: Export declarations are not allowed in the global module fragment
module;
export import syntax_gmf_export_import;
export module syntax_gmf_export_import;
