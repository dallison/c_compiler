// RUN: -std=c++20
// EXPECT: Conflicting declaration of namespace alias 'conflicting_name'

namespace target {}

int conflicting_name;
namespace conflicting_name = target;
