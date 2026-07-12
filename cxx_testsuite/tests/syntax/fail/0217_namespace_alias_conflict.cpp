// RUN: -std=c++20
// EXPECT: Conflicting declaration of namespace alias 'alias_name'

namespace first {}
namespace second {}

namespace alias_name = first;
namespace alias_name = second;
