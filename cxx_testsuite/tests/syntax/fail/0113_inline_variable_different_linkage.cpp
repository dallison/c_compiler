// RUN: -std=c++20
// EXPECT: redeclared with different linkage
inline int inline_linkage_conflict = 1;
static int inline_linkage_conflict;
