// RUN: -std=c++11

void while_null() {
  while (true);
}

void while_empty_compound() {
  while (true) {}
}

void do_null() {
  do; while (true);
}

void do_empty_compound() {
  do {} while (true);
}

void for_null() {
  for (;;);
}

void for_empty_compound() {
  for (;;) {}
}

void for_with_initializer() {
  for (int value = 0;;) {}
}
