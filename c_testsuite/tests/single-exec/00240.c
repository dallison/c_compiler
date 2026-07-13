void _Exit(int status);
long write(int fd, const void* buffer, unsigned long count);

static int state;

__attribute__((constructor(101))) static void first_constructor(void) {
  state = 1;
}

__attribute__((constructor(200))) static void second_constructor(void) {
  if (state == 1) {
    state = 2;
  }
}

__attribute__((destructor(101))) static void last_destructor(void) {
  if (state == 4) {
    write(1, "init/fini attributes ok\n", 24);
    _Exit(0);
  }
  _Exit(1);
}

__attribute__((destructor(200))) static void first_destructor(void) {
  if (state == 3) {
    state = 4;
  }
}

int main(void) {
  if (state != 2) {
    return 2;
  }
  state = 3;
  return 0;
}
