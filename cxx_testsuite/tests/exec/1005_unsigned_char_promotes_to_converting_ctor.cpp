// RUN: -std=c++17
// EXPECT_EXIT: 0

// An unsigned char promotes to int. That promotion outranks a same-width
// signedness conversion to char, so a class with both constructors is
// reachable from unsigned char. absl::Substitute's Arg is this shape, and
// upb passes a descriptor type field as unsigned char.

struct Arg {
  int kind;
  Arg(char) : kind(1) {}
  Arg(int v) : kind(2 + v) {}
  Arg(unsigned int) : kind(9) {}
};

int take(const Arg& arg) { return arg.kind; }

int main() {
  unsigned char field = 4;
  if (take(field) != 6) {
    return 1;
  }
  char letter = 'A';
  if (take(letter) != 1) {
    return 2;
  }
  return 0;
}
