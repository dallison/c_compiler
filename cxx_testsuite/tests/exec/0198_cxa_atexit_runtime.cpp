// RUN: -std=c++20

extern "C" int atexit(void (*)(void));
extern "C" int __cxa_atexit(void (*)(void*), void*, void*);
extern "C" void __cxa_finalize(void*);

static char order[8];
static int order_length;
static int dso_one;
static int dso_two;

void record_plain() {
  order[order_length++] = 'P';
}

void record_argument(void* argument) {
  order[order_length++] = *(char*)argument;
}

void record_late() {
  order[order_length++] = 'L';
}

void register_during_finalize() {
  order[order_length++] = 'R';
  atexit(record_late);
}

int main() {
  static char a = 'A';
  static char b = 'B';
  static char c = 'C';
  if (atexit(record_plain) != 0 ||
      __cxa_atexit(record_argument, &a, &dso_one) != 0 ||
      __cxa_atexit(record_argument, &b, &dso_two) != 0 ||
      __cxa_atexit(record_argument, &c, &dso_one) != 0) {
    return 1;
  }

  __cxa_finalize(&dso_one);
  if (order_length != 2 || order[0] != 'C' || order[1] != 'A') {
    return 2;
  }
  __cxa_finalize(&dso_one);
  if (order_length != 2) {
    return 3;
  }
  if (atexit(register_during_finalize) != 0) {
    return 4;
  }
  __cxa_finalize(nullptr);
  if (order_length != 6 || order[2] != 'R' || order[3] != 'B' ||
      order[4] != 'P' || order[5] != 'L') {
    return 5;
  }
  return 0;
}
