// RUN: -std=c++20
#include <utility>

struct MoveOnlyCopyRejected {
  MoveOnlyCopyRejected(void) {
  }

  MoveOnlyCopyRejected(const MoveOnlyCopyRejected& other) = delete;

  MoveOnlyCopyRejected(MoveOnlyCopyRejected&& other) {
  }
};

int main(void) {
  MoveOnlyCopyRejected value;
  MoveOnlyCopyRejected copy(value);
  return 0;
}
