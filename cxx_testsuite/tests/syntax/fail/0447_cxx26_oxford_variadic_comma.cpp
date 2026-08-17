// RUN: -std=c++26 -Werror=deprecated-declarations
// EXPECT: non-comma-separated ellipsis parameters are deprecated in C++26

void old_variadic_spelling(int...);
void old_pack_and_variadic_spelling(auto......);
