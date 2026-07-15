// RUN: -std=c++20
// EXPECT: Module declaration cannot appear after import declarations
import syntax_impl_order;

module syntax_impl_order;

int x;
