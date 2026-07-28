// RUN: -std=c++20
// EXPECT: Invalid preprocessor directive elifdef

#if 1
#elifdef FEATURE
#endif
