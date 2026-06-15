// RUN: -std=c++17
class ConstBox {
  int value;
public:
  int get(void) const;
  int set(int next);
};

int ConstBox::get(void) const {
  return value;
}

int ConstBox::set(int next) {
  value = next;
  return value;
}

int read_const_box(const ConstBox *box) {
  return sizeof(box->get());
}

int main(void) {
  ConstBox box;
  return sizeof(box.get()) + sizeof(box.set(1));
}
