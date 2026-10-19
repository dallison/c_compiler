// RUN: -std=c++26
// EXPECT_EXIT: 0

template <class T>
struct Box {
  T value;
};

constexpr auto box_info = ^^Box<int>;
using SplicedBox = typename [:box_info:];

int Read(SplicedBox box) { return box.value; }

SplicedBox global_box{7};

int main() {
  SplicedBox box{42};
  if (box.value != 42) return 1;
  typename [:box_info:] direct{5};
  if (direct.value + Read(direct) != 10) return 2;
  if (global_box.value != 7) return 3;
  return 0;
}
