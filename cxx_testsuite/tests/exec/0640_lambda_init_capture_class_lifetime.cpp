// RUN: -std=c++20
// EXPECT_EXIT: 0

// An init-capture of a class with a user-provided constructor is constructed
// once in the closure and destroyed with it.
struct lifetime {
  int* alive;
  explicit lifetime(int* count) : alive(count) { ++*alive; }
  lifetime(const lifetime& other) : alive(other.alive) { ++*alive; }
  ~lifetime() { --*alive; }
};

int main() {
  int alive = 0;
  {
    auto closure = [a = lifetime(&alive)] { return 6; };
    if (closure() != 6) return 9;
    if (alive != 1) return 8;
  }
  return alive;
}
