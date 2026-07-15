// RUN: -std=c++20
// EXPECT: Private module fragment requires a preceding module declaration
module :private;

int orphan_private;
