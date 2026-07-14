// RUN: -std=c++20

#include <any>

struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&&) = default;
};

std::any value = MoveOnly();
