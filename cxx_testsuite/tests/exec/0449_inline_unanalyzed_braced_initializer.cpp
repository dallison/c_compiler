// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0

// Inlining copies a function body in whatever shape it currently has, and a
// member of a class template can be reached from a caller that is analyzed
// before the member itself is.  The copy then holds the initializer as the
// parser left it, with a plain expression per entry and no type, while the
// analysis that follows lowers the body the function keeps and never sees the
// copy.  Code generation places a struct's entries by their designators, so it
// asserted on the copy instead of compiling it.
//
// A table of function pointers behind a static accessor is the shape that
// reached it, which is how std::function, std::any and std::move_only_function
// each dispatch, so none of them compiled at -O2.
//
// The initializer being for a struct is what matters.  An array's entries are
// positional and code generation does handle those, so this needs the struct.

extern "C" int printf(const char*, ...);

struct Ops {
  int (*add)(int, int);
  int (*mul)(int, int);
  const char* name;
};

static int add_impl(int a, int b) { return a + b; }
static int mul_impl(int a, int b) { return a * b; }

template <class T>
struct Dispatch {
  static int identity(int a, int) { return a; }

  static const Ops* table() {
    static const Ops value = {&add_impl, &mul_impl, "ops"};
    return &value;
  }

  // A second table whose entries include a member of the template itself.
  static const Ops* self_table() {
    static const Ops value = {&identity, &mul_impl, "self"};
    return &value;
  }
};

int main() {
  const Ops* ops = Dispatch<int>::table();
  if (ops->add(3, 4) != 7) {
    return 1;
  }
  if (ops->mul(3, 4) != 12) {
    return 2;
  }
  if (ops->name[0] != 'o') {
    return 3;
  }

  const Ops* self = Dispatch<int>::self_table();
  if (self->add(9, 5) != 9) {
    return 4;
  }
  if (self->name[0] != 's') {
    return 5;
  }

  // The same accessor reached through a second instantiation.
  const Ops* other = Dispatch<char>::table();
  if (other->add(1, 1) != 2) {
    return 6;
  }
  return 0;
}
