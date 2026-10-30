// RUN: -std=c++23
// EXPECT: start_lifetime_as requires an implicit-lifetime type

#include <memory>

struct NotImplicit {
  NotImplicit();
  ~NotImplicit();
  int value;
};

void start(void* storage) {
  std::start_lifetime_as<NotImplicit>(storage);
}
