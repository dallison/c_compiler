// RUN: -std=c++17
// EXPECT_EXIT: 0

struct MoveOnly {
  int value;
  int* self;
  MoveOnly(int v) : value(v), self(&value) {}
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&&) = delete;
};

static MoveOnly Make() { return MoveOnly(7); }

int main() {
  MoveOnly data = Make();
  if (data.value != 7) return 1;
  if (data.self != &data.value) return 2;
  return 0;
}
