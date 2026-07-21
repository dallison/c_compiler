// RUN: -target 65c02
// EXPECT: cannot use 'try' with exception handling disabled

int catch_on_65c02() {
  try {
    return 1;
  } catch (...) {
    return 2;
  }
}
