// RUN: -std=c++17
// EXPECT: Cannot call non-const member function set on const object
class ConstBox {
public:
  int set(int next);
};

int ConstBox::set(int next) {
  return next;
}

int use_const_box(const ConstBox *box) {
  return sizeof(box->set(1));
}
