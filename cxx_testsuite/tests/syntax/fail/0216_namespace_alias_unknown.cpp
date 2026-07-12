// RUN: -std=c++20
// EXPECT: Unknown namespace namespace_that_does_not_exist

namespace missing_alias = namespace_that_does_not_exist;
