// RUN: -std=c++26
// EXPECT: function type alias

using invalid_alias = int() pre (true);
