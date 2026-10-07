// RUN: -std=c++17 -fconstexpr-eval=audit
// Members reached through a constant pointer: `p->m` and `(*p).m` as values
// and as addresses, where the pointer is a constexpr variable, a stored `this`,
// or names a non-const global (whose address, but not value, is constant).

struct G {
  const G* self;
  int v = 3;
  int w = 4;
  constexpr G() : self(this) {}
};

constexpr G g;
constexpr G g2;
static_assert(g.self == &g, "self");
static_assert(g.self != &g2, "distinct objects");
static_assert(g.self != nullptr, "non-null");
static_assert(g.self->w == 4, "read through stored this");
static_assert(&g.self->w == &g.w, "member address through stored this");
static_assert(&g.self->v != &g.w, "distinct members");

struct P {
  int v = 3;
  int w = 4;
};

constexpr P p;
constexpr const P* pp = &p;
static_assert(pp->w == 4, "arrow read");
static_assert((*pp).w == 4, "contents read");
static_assert(&pp->w == &p.w, "arrow address");
static_assert(&(*pp).v == &p.v, "contents address");

P mutable_p;
constexpr P* pm = &mutable_p;
static_assert(&pm->w == &mutable_p.w, "address of member of a global");
static_assert(&(*pm).v == &mutable_p.v, "address of member of a global");

int main() { return 0; }
