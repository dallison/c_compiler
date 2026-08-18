// RUN: -std=c++26
// EXPECT: 'indeterminate' attribute takes no arguments
// EXPECT: 'indeterminate' attribute applies only to parameters and automatic block variables
// EXPECT: 'indeterminate' on a parameter must appear on the first declaration

[[indeterminate(1)]] int global_value;

void redeclared(int value);
void redeclared([[indeterminate]] int value) {}
