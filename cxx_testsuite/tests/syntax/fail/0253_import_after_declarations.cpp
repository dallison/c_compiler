// RUN: -std=c++20
// EXPECT: Import declaration must appear before other declarations
export module syntax_import_order;

int first_decl = 0;
import syntax_import_order;
