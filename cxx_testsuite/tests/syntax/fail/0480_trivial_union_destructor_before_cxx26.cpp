// RUN: -std=c++23
// EXPECT: deleted

struct nontrivial {
  ~nontrivial();
};

union storage {
  nontrivial object;
  int integer;

  storage() : integer(0) {}
};

void use_storage() {
  storage value;
}
