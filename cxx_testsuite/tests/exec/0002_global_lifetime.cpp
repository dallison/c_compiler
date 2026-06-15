int putchar(int c) asm("putchar");

int state;

struct GlobalBox {
  GlobalBox();
  ~GlobalBox();
  int value;
};

GlobalBox::GlobalBox() {
  value = 13;
  state = 5;
  putchar('C');
}

GlobalBox::~GlobalBox() {
  state = 9;
  putchar('D');
  putchar('\n');
}

GlobalBox global_box;

int main(void) {
  if (state != 5) {
    return 1;
  }
  if (global_box.value != 13) {
    return 2;
  }
  putchar('M');
  state = 0;
  return state;
}
