// RUN: -target 65c02 -fexceptions

int catch_on_65c02() {
  try {
    return 1;
  } catch (...) {
    return 2;
  }
}
