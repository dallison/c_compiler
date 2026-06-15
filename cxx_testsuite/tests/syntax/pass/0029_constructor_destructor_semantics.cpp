// RUN: -std=c++17
struct Box {
  Box(void);
  Box(int v);
  Box(char v);
  ~Box(void);
  int value(void) const;
};

Box::Box(void) {
}

Box::Box(int v) {
}

Box::Box(char v) {
}

Box::~Box(void) {
}

int Box::value(void) const {
  return 1;
}

int main(void) {
  char c;
  Box default_box;
  Box int_box(1);
  Box char_box(c);
  return default_box.value() + int_box.value() + char_box.value();
}
