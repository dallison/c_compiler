// RUN: -std=c++23
// EXPECT: deleted

struct nontrivial {
  nontrivial();
  ~nontrivial();
};

union storage {
  nontrivial object;
  int integer;
};

void use_storage() {
  storage value;
}
