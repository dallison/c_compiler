// RUN: -std=c++20

// Class objects created by new-expressions, default-initialized local arrays
// of class type, and class members initialized from braced lists, all within
// constant evaluation.

struct A { int a; int b; };
struct L { int a = 3; };
struct J { int a; constexpr J(int v) : a(v) {} };
struct M { int a; constexpr M() : a(5) {} };

constexpr int NewAggregate() { A* p = new A{1, 2}; int v = p->b; delete p; return v; }
static_assert(NewAggregate() == 2);
constexpr int NewDefault() { L* p = new L; int v = p->a; delete p; return v; }
static_assert(NewDefault() == 3);
constexpr int NewValueInit() { A* p = new A(); int v = p->a + p->b; delete p; return v; }
static_assert(NewValueInit() == 0);
constexpr int NewConstructor() { J* p = new J(2); int v = p->a; delete p; return v; }
static_assert(NewConstructor() == 2);
constexpr int NewDefaultConstructor() { M* p = new M; int v = p->a; delete p; return v; }
static_assert(NewDefaultConstructor() == 5);
constexpr int NewArray() {
  A* p = new A[2]{{1, 2}, {3, 4}};
  int v = p[1].b;
  delete[] p;
  return v;
}
static_assert(NewArray() == 4);

struct N { int v; N* next; };
constexpr int LinkedList() {
  N* head = nullptr;
  for (int i = 1; i <= 3; i++) head = new N{i, head};
  int sum = 0;
  while (head) {
    N* n = head;
    sum += n->v;
    head = n->next;
    delete n;
  }
  return sum;
}
static_assert(LinkedList() == 6);

constexpr int LocalArray() { L ls[3]; return ls[2].a; }
static_assert(LocalArray() == 3);
constexpr int LocalArray2D() { L ls[2][3]; return ls[1][2].a; }
static_assert(LocalArray2D() == 3);
constexpr int LocalConstructedArray() { M ms[2]; return ms[1].a; }
static_assert(LocalConstructedArray() == 5);
struct B { int b = 4; };
struct D : B { int d = 5; };
constexpr int LocalDerivedArray() { D ds[2]; return ds[1].b + ds[1].d; }
static_assert(LocalDerivedArray() == 9);
constexpr int LocalArrayWrite() { L ls[3]; ls[2].a = 4; return ls[2].a; }
static_assert(LocalArrayWrite() == 4);

struct O { A x; constexpr O() : x{1, 2} {} };
constexpr O o;
static_assert(o.x.b == 2);
constexpr int BracedMember() { O l; return l.x.b; }
static_assert(BracedMember() == 2);

int main() { return 0; }
