// RUN: -std=c++20
//
// Transitive base matching: a three-level hierarchy (mirroring the standard
// exception hierarchy, e.g. out_of_range -> logic_error -> exception) caught by
// a handler naming the top-most base.

struct GrandBase {
  int g;
};

struct Middle : GrandBase {
  int m;
};

struct Leaf : Middle {
  int l;
};

int main(void) {
  try {
    Leaf leaf;
    leaf.g = 5;
    leaf.m = 6;
    leaf.l = 7;
    throw leaf;
    return 1;
  } catch (GrandBase& gb) {
    return gb.g - 5;  // 0 on success
  } catch (...) {
    return 2;
  }
}
