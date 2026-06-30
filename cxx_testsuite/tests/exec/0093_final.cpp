// 'final' on classes and virtual functions compiles and runs correctly, and
// 'final' remains usable as an ordinary identifier when it is not in the
// class-virt-specifier position.

// A final class (no base clause): instantiation and use still work.
struct Sealed final {
  int v;
  int get() { return v; }
};

// A final class with a base clause: still a normal derived class itself.
struct Base {
  virtual int who() { return 1; }
};
struct Mid : public Base {
  // A final overrider: cannot be overridden further, but dispatch still works.
  int who() final { return 2; }
};

int main() {
  Sealed s;
  s.v = 41;
  if (s.get() != 41) {
    return 1;
  }

  Mid m;
  Base* bp = &m;
  if (bp->who() != 2) {
    return 2;
  }

  // 'final' is only a contextual keyword; it is a legal identifier elsewhere.
  int final = 7;
  if (final != 7) {
    return 3;
  }

  return 0;
}
