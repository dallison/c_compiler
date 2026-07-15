// RUN: -std=c++20
// EXPECT: Global module fragment must be the first declaration
int prelude = 0;
module;
export module syntax_gmf_order;
