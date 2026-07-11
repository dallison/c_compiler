// RUN: -std=c++20
// EXPECT_EXIT: 0

struct Box {
  Box(int) {}
};

int pick(int, ...) { return 1; }
int pick(int, long) { return 2; }

int convert(int, ...) { return 3; }
int convert(int, Box) { return 4; }

int fallback(int, ...) { return 5; }

struct Receiver {
  int call(int, ...) { return 6; }
  int call(int, long) { return 7; }
};

int main() {
  if (pick(1, 2) != 2) {
    return 1;
  }
  if (convert(1, 2) != 4) {
    return 2;
  }
  if (fallback(1, (int*)0) != 5) {
    return 3;
  }
  Receiver r;
  if (r.call(1, 2) != 7) {
    return 4;
  }
  return 0;
}
