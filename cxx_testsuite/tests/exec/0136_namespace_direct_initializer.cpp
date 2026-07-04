// RUN: -std=c++20
// EXPECT_EXIT: 0

struct Tag {
  int value;

  explicit Tag(int v) : value(v) {}
};

const Tag global_tag(42);

int main(void) {
  if (global_tag.value != 42) {
    return 1;
  }
  return 0;
}
