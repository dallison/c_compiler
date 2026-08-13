// RUN: -std=c++26
// EXPECT_EXIT: 0

struct triple {
  int first;
  int second;
  int third;
};

struct placeholder_members {
  int _;
  long _;
};

struct lifetime {
  int* alive;

  explicit lifetime(int* count) : alive(count) { ++*alive; }
  lifetime(const lifetime& other) : alive(other.alive) { ++*alive; }
  ~lifetime() { --*alive; }
};

int main() {
  static_assert(__cpp_placeholder_variables == 202306L);

  int _ = 7;
  if (_ != 7) {
    return 1;
  }
  int _ = 9;

  auto [_, _, kept] = triple{1, 2, 3};
  if (kept != 3) {
    return 2;
  }

  placeholder_members members{4, 5};
  if (sizeof(members) < sizeof(int) + sizeof(long)) {
    return 3;
  }

  int alive = 0;
  {
    lifetime _(&alive);
    lifetime _(&alive);
    if (alive != 2) {
      return 4;
    }
  }
  if (alive != 0) {
    return 5;
  }

  {
    auto closure = [_ = lifetime(&alive), _ = lifetime(&alive)] {
      return 6;
    };
    if (alive != 2 || closure() != 6) {
      return 6;
    }
  }
  return alive;
}
