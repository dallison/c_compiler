// RUN: -std=c++11
// EXPECT: 'deprecated' attribute requires C++14

[[deprecated]] void old_function();
