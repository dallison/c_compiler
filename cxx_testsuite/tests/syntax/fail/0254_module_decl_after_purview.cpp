// RUN: -std=c++20
// EXPECT: Module declaration must appear before other declarations
int before_module = 0;
export module syntax_late_decl;
