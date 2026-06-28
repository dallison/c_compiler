// RUN: -std=c++20
#include <memory>

int main(void) {
  std::unique_ptr<int> first(new int(1));
  std::unique_ptr<int> second(first);
  return 0;
}
