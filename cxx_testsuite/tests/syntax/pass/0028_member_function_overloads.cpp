// RUN: -std=c++17
struct Box {
  int value(int v);
  char value(char v);
  int *choose(int *p);
  char *choose(char *p);
  int read(void);
  int read(void) const;
  static int stat(int v);
  static char stat(char v);
};

int Box::value(int v) {
  return v;
}

char Box::value(char v) {
  return v;
}

int *Box::choose(int *p) {
  return p;
}

char *Box::choose(char *p) {
  return p;
}

int Box::read(void) {
  return 1;
}

int Box::read(void) const {
  return 2;
}

int Box::stat(int v) {
  return v;
}

char Box::stat(char v) {
  return v;
}

int main(void) {
  Box box;
  const Box const_box;
  char c;
  int i;
  char *cp;
  int *ip;
  char selected_char = box.value(c);
  int selected_int = box.value(i);
  char *selected_char_pointer = box.choose(cp);
  int *selected_int_pointer = box.choose(ip);
  int non_const_read = box.read();
  int const_read = const_box.read();
  char selected_static_char = box.stat(c);
  int selected_static_int = box.stat(i);
  return sizeof(selected_char) + sizeof(selected_int) +
         sizeof(selected_char_pointer) + sizeof(selected_int_pointer) +
         sizeof(non_const_read) + sizeof(const_read) +
         sizeof(selected_static_char) + sizeof(selected_static_int);
}
