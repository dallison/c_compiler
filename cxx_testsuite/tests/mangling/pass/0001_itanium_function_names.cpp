// RUN: -std=c++17
// EXPECT-ASM: _ZN2ns1fEi
// EXPECT-ASM: _ZN2ns1fEj
// EXPECT-ASM: _ZN3BoxC1Ev
// EXPECT-ASM: _ZN3BoxD1Ev
// EXPECT-ASM: _ZN3Box6methodEi
// EXPECT-ASM: _ZNK3Box5valueEv
// EXPECT-ASM: _ZN3Box4statEd
// EXPECT-ASM: _Zpl3Box3Box

namespace ns {
void f(int value) {
}

void f(unsigned int value) {
}
}

struct Box {
  Box();
  ~Box();
  void method(int value);
  int value() const;
  static void stat(double value);
};

Box::Box() {
}

Box::~Box() {
}

void Box::method(int value) {
}

int Box::value() const {
  return 0;
}

void Box::stat(double value) {
}

void operator+(Box left, Box right) {
}

int main(void) {
  return 0;
}
