// RUN: -std=c++20
// EXPECT_EXIT: 0

struct Box {
  Box() : value(7) {}

  int get() const {
    return value;
  }

 private:
  int value;
};

int main() {
  Box box;
  return box.get() == 7 ? 0 : 1;
}
