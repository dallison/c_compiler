int putchar(int c) asm("putchar");

struct LocalBox {
  LocalBox();
  ~LocalBox();
  int value;
};

LocalBox::LocalBox() {
  value = 21;
  putchar('C');
}

LocalBox::~LocalBox() {
  putchar('D');
  putchar('\n');
}

int use_box(void) {
  static LocalBox box;
  putchar('U');
  return box.value;
}

int main(void) {
  if (use_box() != 21) {
    return 1;
  }
  if (use_box() != 21) {
    return 2;
  }
  return 0;
}
