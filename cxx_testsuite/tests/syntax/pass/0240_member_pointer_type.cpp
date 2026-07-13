// RUN: -std=c++20
#include <type_traits>
struct S { int v; };
static_assert(std::is_member_pointer<int (S::*)>::value);
int main() { return 0; }
