// Regression: inline-defined and implicitly-defined constructors must
// initialize __vptr, so virtual dispatch works for stack objects whose
// constructor bodies were built while the class was still being parsed (before
// its vtables were registered).

// Inline-defined constructor in the class body.
struct InlineCtor {
  InlineCtor() {}
  virtual int who() { return 1; }
};
struct InlineDerived : public InlineCtor {
  InlineDerived() {}
  int who() override { return 2; }
};

// Implicitly-defined constructors (none declared).
struct Implicit {
  virtual int who() { return 3; }
};
struct ImplicitDerived : public Implicit {
  int who() override { return 4; }
};

// Inline-defined virtual destructor (shifts the first function-pointer slot).
struct WithDtor {
  WithDtor() {}
  virtual ~WithDtor() {}
  virtual int who() { return 5; }
};
struct WithDtorDerived : public WithDtor {
  WithDtorDerived() {}
  int who() override { return 6; }
};

int main() {
  InlineDerived id;
  InlineCtor* ip = &id;
  if (ip->who() != 2) {
    return 1;
  }

  ImplicitDerived md;
  Implicit* mp = &md;
  if (mp->who() != 4) {
    return 2;
  }

  WithDtorDerived wd;
  WithDtor* wp = &wd;
  if (wp->who() != 6) {
    return 3;
  }

  // Base objects on the stack dispatch to their own overrides too.
  InlineCtor base;
  if (base.who() != 1) {
    return 4;
  }

  return 0;
}
