// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <algorithm>
#include <cstddef>

struct Slot {
  unsigned char byte;
};

int main() {
  std::size_t slot_align = 1;
  std::size_t aligned = (std::max)(slot_align, alignof(Slot));
  std::size_t sized = (std::max)(slot_align, sizeof(Slot));
  if (aligned != 1 || sized != 1) {
    return 1;
  }
  if ((std::max)(sizeof(Slot), sizeof(int)) != sizeof(int)) {
    return 2;
  }
  return 0;
}
