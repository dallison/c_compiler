// RUN: -std=c++20
// EXPECT: Variable has incomplete type 'I'
// EXPECT: Field has incomplete type 'I'
// EXPECT: Field has incomplete type 'Self'
// EXPECT: Field has incomplete type 'Outer'
// EXPECT: Base class has incomplete type 'I'
// EXPECT: Incomplete result type 'I' in function definition
// EXPECT: invalid application of 'sizeof' to an incomplete type 'I'
// EXPECT: invalid application of 'alignof' to an incomplete type 'I'
// EXPECT: Allocation of incomplete type 'I'
// EXPECT: arithmetic on a pointer to an incomplete type 'I'
// EXPECT: member access into incomplete type 'I'
// EXPECT: cannot throw object of incomplete type 'I'
// EXPECT: cannot catch incomplete type 'I'

struct I;

I global;
I array[2];

struct WithMember {
  I member;
};

struct Self {
  Self self;
};

struct Outer {
  struct Inner {
    Outer outer;
  };
};

struct Derived : I {};

I Result() { return *(I*)0; }

void Parameter(I value) {}

int Sizes(I* p) { return sizeof(I) + alignof(I) + sizeof(*p); }

void Allocate() { new I; }

void Arithmetic(I* p, I* q) {
  p++;
  (void)(p + 1);
  (void)(p - q);
}

int Member(I* p) { return p->x; }

void Throw(I* p) { throw *p; }

void Catch() {
  try {
  } catch (I) {
  }
}

int main() {
  I local;
  return 0;
}
