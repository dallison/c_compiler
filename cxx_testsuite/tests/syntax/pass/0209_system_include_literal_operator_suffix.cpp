// RUN: -std=c++20

#include <system_path_udl.hpp>

static_assert(2sysh == 42);
static_assert(operator""sysh(2) == 42);
