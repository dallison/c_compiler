// RUN: -std=c++26

#include <cstring>
#include <utility>

struct pair {
  int first;
  int second;
};

int main() {
  int scalar;
  if (scalar != 0) {
    return 1;
  }

  int values[3];
  values[1] = 17;
  if (values[0] != 0 || values[1] != 17 || values[2] != 0) {
    return 2;
  }

  pair source;
  source.first = 9;
  pair destination;
  std::memcpy(&destination, &source, sizeof(source));
  if (destination.first != 9 || destination.second != 0) {
    return 3;
  }

  std::observable_checkpoint();

  [[indeterminate]] int untouched;
  (void)&untouched;
  return 0;
}
