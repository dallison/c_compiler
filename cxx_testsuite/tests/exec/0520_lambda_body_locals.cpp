// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <stdexcept>
#include <utility>

struct Pair {
  int first;
  int second;
};

int main() {
  int status = 0;
  auto catch_int = [&] {
    try {
      throw 7;
    } catch (int value) {
      status = value;
    }
  };
  catch_int();
  if (status != 7) {
    return 1;
  }

  auto catch_object = [&] {
    try {
      throw std::runtime_error("ok");
    } catch (const std::runtime_error& error) {
      status = error.what()[0];
    }
  };
  catch_object();
  if (status != 'o') {
    return 2;
  }

  int extra = 4;
  auto bind = [&] {
    Pair pair{2, 3};
    auto [left, right] = pair;
    return left + right + extra;
  };
  if (bind() != 9) {
    return 3;
  }
  return 0;
}
