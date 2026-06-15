// RUN: -std=c++17
class Box {
 public:
  Box(void);
  Box(int value);
  Box(int left, int right);
  int value;
};

Box::Box(void) {
  value = 0;
}

Box::Box(int init) {
  value = init;
}

Box::Box(int left, int right) {
  value = left + right;
}

int main(void) {
  Box* default_box = new Box;
  Box* paren_box = new Box(3);
  Box* brace_box = new Box{4, 5};
  delete default_box;
  delete paren_box;
  delete brace_box;
  return 0;
}
