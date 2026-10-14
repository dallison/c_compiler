// RUN: -std=c++17
// EXPECT_EXIT: 0

// A private using-declaration makes a protected base member a private member
// of the derived class, so the derived class's own members may still call it.
// Protobuf's reflection accessors redeclare inherited helpers this way.

struct Accessor {
 protected:
  template <typename T>
  int Add(T v) const { return static_cast<int>(v) + 1; }
};

template <typename T>
class Wrapper : public Accessor {
 protected:
  virtual int Mutable(int x) const { return x * 2; }
};

template <typename T>
class Primitive final : public Wrapper<T> {
  using Wrapper<T>::Mutable;

 public:
  int Use(int x) const { return Mutable(x) + this->Mutable(1); }
};

class Strings final : public Wrapper<char> {
  using Accessor::Add;

 public:
  int Use(int x) const { return Add<int>(x) + Mutable(x); }
};

int main() {
  Primitive<int> p;
  Strings s;
  if (p.Use(3) != 8) return 1;
  if (s.Use(3) != 10) return 2;
  return 0;
}
