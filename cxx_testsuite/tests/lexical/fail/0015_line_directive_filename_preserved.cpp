// RUN: -std=c++26
// EXPECT: first.cpp:20: line marker

#line 10 "first.cpp"
#line 20
#error line marker
