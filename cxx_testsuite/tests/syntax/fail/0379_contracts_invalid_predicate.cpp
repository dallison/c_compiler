// RUN: -std=c++26
// EXPECT: cannot convert from 'no_boolean_conversion' to 'bool'

struct no_boolean_conversion {};

int invalid_predicate()
    pre (no_boolean_conversion{}) {
  return 0;
}
