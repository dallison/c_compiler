// RUN: -std=c++17
// EXPECT_EXIT: 0

struct Printer {
  struct Sub {
    int value;
  };
};

namespace {
using Sub = Printer::Sub;
using Sub = Printer::Sub;

int read(Sub item) { return item.value; }
}  // namespace

int main() {
  Sub item;
  item.value = 4;
  if (read(item) != 4) return 1;
  return 0;
}
