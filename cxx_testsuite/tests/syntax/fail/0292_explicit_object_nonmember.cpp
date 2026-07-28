// RUN: -std=c++23
// EXPECT: explicit object parameter is only allowed in a member function

void invalid(this int value);
