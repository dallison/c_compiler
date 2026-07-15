// RUN: -std=c++20
// Each specialization of a conversion operator template must mangle to a
// distinct symbol carrying its deduced template argument, so multiple
// instantiations (operator int / long / double) never collide.  davecc encodes
// the conversion operator as the source-name `operator T` followed by the
// template-argument list <int>/<long>/<double>.
// EXPECT-ASM: _ZNK1S10operator_TEIiEv
// EXPECT-ASM: _ZNK1S10operator_TEIlEv
// EXPECT-ASM: _ZNK1S10operator_TEIdEv
struct S {
  int v;
  template <class T>
  operator T() const {
    return (T)v;
  }
};

int use() {
  S s{1};
  int a = s;
  long b = s;
  double d = s;
  return a + (int)b + (int)d;
}
