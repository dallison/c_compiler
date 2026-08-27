// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0

// The SSA name of a variable stands for its storage, and its lattice value in
// the conditional constant propagator is what a load through it reads.  An
// address expression built on that name -- `adda(ssavar, offset)` reaching a
// member -- consumes the storage, not the contents, so the contents must not be
// folded in as an address.  Initializing a local to a constant and then reading
// a member back through it used to produce a load from the constant itself,
// which is address zero whenever the initializer is zero.

struct one_member {
  int value = 0;

  void add(int increment) { value += increment; }
};

struct two_members {
  int first;
  int second;
};

struct nested {
  one_member inner;
  int trailing;
};

int main() {
  one_member zero_initialized{0};
  if (zero_initialized.value != 0) {
    return 1;
  }

  one_member counted{0};
  counted.add(5);
  if (counted.value != 5) {
    return 2;
  }

  // A member past the first, so the folded address would be the offset rather
  // than zero.
  two_members pair{0, 7};
  if (pair.first != 0 || pair.second != 7) {
    return 3;
  }

  nested outer{{0}, 9};
  if (outer.inner.value != 0 || outer.trailing != 9) {
    return 4;
  }

  return 0;
}
