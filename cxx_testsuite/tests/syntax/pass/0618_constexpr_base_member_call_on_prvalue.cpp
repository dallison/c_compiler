// RUN: -std=c++20 -fconstexpr-eval=audit

// A member of a base class at offset 0 called on a class prvalue binds `this`
// to the base subobject of the temporary, also through an explicit
// `static_cast` to a base reference, in both constant evaluators.
struct Empty {
  constexpr int answer() const { return 42; }
};

struct Base {
  int tag;
  constexpr int get() const { return tag; }
};

struct Other {
  short x;
  constexpr int get_x() const { return x; }
};

struct Many : Empty, Base, Other {
  int c;
};

constexpr Many make_many() { return Many{{}, {7}, {8}, 10}; }
constexpr Many many = make_many();

static_assert(many.answer() == 42);
static_assert(Many{{}, {7}, {8}, 10}.answer() == 42);
static_assert(make_many().answer() == 42);
static_assert(make_many().get() == 7);
static_assert(make_many().get_x() == 8);
static_assert(static_cast<const Empty&>(make_many()).answer() == 42);
static_assert(static_cast<const Base&>(make_many()).tag == 7);
static_assert(static_cast<const Base&>(make_many()).get() == 7);
static_assert(static_cast<const Other&>(make_many()).get_x() == 8);

// A CRTP base at offset 0 casts `this` back to the derived class.
template <class D>
struct Interface {
  constexpr D& derived() { return static_cast<D&>(*this); }
  constexpr int at(int i) { return derived().p[i]; }
};

struct View : Interface<View> {
  int* p;
  constexpr View(int* first) : p(first) {}
};

constexpr bool crtp_local() {
  int values[] = {2, 4, 6};
  View view(values);
  return view.at(1) == 4;
}
static_assert(crtp_local());

int main() { return 0; }
