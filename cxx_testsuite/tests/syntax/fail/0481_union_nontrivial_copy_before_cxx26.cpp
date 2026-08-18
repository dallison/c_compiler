// RUN: -std=c++23
// EXPECT: deleted

struct nontrivial {
  nontrivial();
  nontrivial(const nontrivial&);
  nontrivial& operator=(const nontrivial&);
  ~nontrivial();
};

union storage {
  nontrivial object;
  int integer;

  storage() : integer(0) {}
  ~storage() {}
};

void copy_storage() {
  storage source;
  storage copy(source);
  copy = source;
}
