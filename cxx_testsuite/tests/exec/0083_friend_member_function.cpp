// RUN: -std=c++20
// EXPECT_EXIT: 0
//
// A specific member function of another class can be granted friendship with
// 'friend ret Class::method(...);'.

struct Data;

struct Accessor {
  int total(const Data& d) const;
};

struct Data {
 private:
  int a;
  int b;

 public:
  Data(int x, int y) : a(x), b(y) {}
  // Grant friendship to one specific member function of Accessor.
  friend int Accessor::total(const Data& d) const;
};

int Accessor::total(const Data& d) const { return d.a + d.b; }

int main(void) {
  Data d(4, 5);
  Accessor acc;
  return acc.total(d) == 9 ? 0 : 1;
}
