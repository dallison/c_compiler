// RUN: -std=c++20
// EXPECT: namespace alias 'alias_name' cannot be extended

namespace target {}
namespace alias_name = target;

namespace alias_name {
int invalid;
}
