// RUN: -std=c++20
// EXPECT_EXIT: 0

// Front-end de-virtualization: a virtual call can be bound statically when the
// method is final, the static class is final, or the object expression is a
// complete object of that class.  Pointers and references to a non-final class
// must still dispatch on the dynamic type.

struct Base {
  virtual int id() { return 1; }
  virtual ~Base() {}
};

struct Mid : Base {
  int id() final override { return 2; }
};

struct Leaf : Mid {};

struct FinalDerived final : Base {
  int id() override { return 3; }
};

struct Holder {
  FinalDerived member;
  Base& ref;
  explicit Holder(Base& r) : ref(r) {}
};

int through_base_ptr(Base* p) { return p->id(); }
int through_base_ref(Base& r) { return r.id(); }
int through_mid_ptr(Mid* p) { return p->id(); }
int through_final_ptr(FinalDerived* p) { return p->id(); }
int through_value(FinalDerived v) { return v.id(); }

int main(void) {
  Leaf leaf;
  FinalDerived fin;
  Base base;

  if (through_base_ptr(&leaf) != 2) {
    return 1;
  }
  if (through_base_ptr(&fin) != 3) {
    return 2;
  }
  if (through_base_ptr(&base) != 1) {
    return 3;
  }
  if (through_base_ref(leaf) != 2) {
    return 4;
  }
  if (through_base_ref(fin) != 3) {
    return 5;
  }

  if (through_mid_ptr(&leaf) != 2) {
    return 6;
  }
  if (through_final_ptr(&fin) != 3) {
    return 7;
  }

  if (fin.id() != 3) {
    return 8;
  }
  if (FinalDerived().id() != 3) {
    return 9;
  }
  if (through_value(fin) != 3) {
    return 10;
  }

  Holder holder(leaf);
  if (holder.member.id() != 3) {
    return 11;
  }
  if (holder.ref.id() != 2) {
    return 12;
  }

  FinalDerived arr[2];
  if (arr[1].id() != 3) {
    return 13;
  }

  return 0;
}
