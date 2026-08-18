// RUN: -std=c++26
// EXPECT: requires an implicit-lifetime aggregate

#include <memory>

struct nonaggregate {
  nonaggregate();
  int value;
};

void invalid_start(nonaggregate& value) {
  std::start_lifetime(value);
}
