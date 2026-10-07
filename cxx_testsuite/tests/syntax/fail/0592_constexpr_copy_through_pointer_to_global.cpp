// RUN: -std=c++17 -fconstexpr-eval=audit
// EXPECT: constexpr variable initializer is not a constant expression

// A pointer to a non-const global is a constant, and so is the address of a
// member reached through it, but the object's value is not: copying it out
// is ill-formed whether the pointer is a constexpr variable or a parameter.

struct In {
  int a = 1;
};

struct P {
  int v = 3;
  In in;
};

P mutable_p;
constexpr P* pm = &mutable_p;
constexpr const In* pi = &pm->in;

constexpr P Copy(const P* p) { return *p; }

constexpr P whole = *pm;
constexpr In member = (*pm).in;
constexpr P copied = Copy(&mutable_p);

int main() { return 0; }
