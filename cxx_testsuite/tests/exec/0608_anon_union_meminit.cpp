// RUN: -std=c++17
// EXPECT_EXIT: 0

struct CordRep {};

struct Rep {
  struct AsTree {
    explicit constexpr AsTree(CordRep* tree) : rep(tree) {}
    long info = 1;
    CordRep* rep;
  };

  explicit Rep(int) {}
  constexpr Rep() : data{0} {}
  explicit constexpr Rep(CordRep* rep) : as_tree(rep) {}
  explicit constexpr Rep(char c)
      : data{c, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15} {}

  union {
    char data[16];
    AsTree as_tree;
  };
};

int main() {
  CordRep node;
  Rep a;
  if (a.data[0] != 0) {
    return 1;
  }
  Rep b(&node);
  if (b.as_tree.rep != &node) {
    return 2;
  }
  Rep c(7);
  return c.data[0] == 7 ? 0 : 3;
}
