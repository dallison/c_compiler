// RUN: -std=c++20
// Itanium mangling of operator<=> (encoded `ss`), member and free forms.
// EXPECT-ASM: _ZNK3BoxssERK3Box
// EXPECT-ASM: _ZssRK3Boxi
struct Box {
  int value;
  int operator<=>(const Box& other) const;
};

int Box::operator<=>(const Box& other) const {
  return value - other.value;
}

int operator<=>(const Box& a, int b) {
  return a.value - b;
}
