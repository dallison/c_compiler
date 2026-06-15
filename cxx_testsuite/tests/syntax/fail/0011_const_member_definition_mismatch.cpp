// RUN: -std=c++17
// EXPECT: Symbol get redeclared with different type
class ConstBox {
public:
  int get(void) const;
};

int ConstBox::get(void) {
  return 1;
}
