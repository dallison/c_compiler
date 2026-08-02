// RUN: -std=c++23
// EXPECT: static lambda cannot be mutable

auto function = [] static mutable { return 1; };
