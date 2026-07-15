// RUN: -std=c++20
// EXPECT: Import declaration is not allowed in the global module fragment
module;
import syntax_gmf_import;
export module syntax_gmf_import;
