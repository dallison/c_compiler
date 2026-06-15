// RUN: -std=c++17
// EXPECT: Cannot assign to this expression
class ConstBox {
  int value;
public:
  int set_bad(int next) const;
};

int ConstBox::set_bad(int next) const {
  value = next;
  return value;
}
