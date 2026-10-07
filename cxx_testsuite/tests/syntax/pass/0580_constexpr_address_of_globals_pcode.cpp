// RUN: -std=c++20 -fconstexpr-eval=pcode

// 0579 with the p-code evaluator: the address of an object with static
// storage duration is a constant expression even when the object is not.

int g;
int h[3];
struct S {
  int a;
  int b;
} s;

constexpr int* ps[] = {&g, &h[1]};
constexpr int* const* pps = &ps[1];

struct Refs {
  int* p;
  int* q;
};
constexpr Refs refs = {&s.b, h + 2};

constexpr bool NonNull(const int* p) { return p != nullptr; }
constexpr bool Same(const int* p, const int* q) { return p == q; }
constexpr const int* Next(const int* p) { return p + 1; }

static_assert(ps[0] == &g);
static_assert(ps[1] == &h[1]);
static_assert(*pps == &h[1]);
static_assert(refs.p == &s.b && refs.q == &h[2]);
static_assert(NonNull(&g));
static_assert(Same(&h[0] + 1, &h[1]));
static_assert(Next(&h[0]) == &h[1]);
static_assert(!Same(&g, &h[0]));
