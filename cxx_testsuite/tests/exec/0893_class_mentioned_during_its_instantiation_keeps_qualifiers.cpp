// A mention of a class while it is still being instantiated shares the type
// being built, but keeps its own cv-qualifiers: `const Info&` in a member of
// a local class of a function template is still const.
#include <vector>

template <typename T>
int Run() {
  struct Info {
    T id;
    std::vector<Info> nested;
    static int Id(const Info& info) { return info.id; }
    static int Id(Info& info) { return -info.id; }
    int Probe() const {
      const Info& self = *this;
      return Id(self);
    }
  };
  Info leaf{3, {}};
  const Info& cref = leaf;
  if (Info::Id(cref) != 3 || Info::Id(leaf) != -3) return 1;
  if (leaf.Probe() != 3) return 2;
  return 0;
}

template <typename T>
struct Outer {
  struct Inner {
    T v;
    std::vector<Inner> kids;
    static int Id(const Inner& i) { return i.v; }
    static int Id(Inner& i) { return -i.v; }
  };
};

int main() {
  if (int r = Run<int>()) return r;
  Outer<int>::Inner in{4, {}};
  const Outer<int>::Inner& ci = in;
  if (Outer<int>::Inner::Id(ci) != 4) return 3;
  if (Outer<int>::Inner::Id(in) != -4) return 4;
  return 0;
}
