// RUN: -std=c++20
// EXPECT: Module declaration cannot appear after import declarations
import syntax_nonmod;

int user_code() { return 0; }

module syntax_nonmod;
