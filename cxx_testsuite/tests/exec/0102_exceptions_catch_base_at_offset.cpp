// RUN: -std=c++20
//
// Catching a base subobject that lives at a non-zero offset (the second base of
// a multiply-derived class) must adjust the exception pointer to that subobject.

struct First {
  long a;
};

struct Second {
  int b;
};

struct Both : First, Second {
  int c;
};

int main(void) {
  try {
    Both obj;
    obj.a = 1;
    obj.b = 99;
    obj.c = 3;
    throw obj;
    return 1;
  } catch (Second& s) {
    return s.b - 99;  // 0 on success; wrong offset would read a or c
  } catch (...) {
    return 2;
  }
}
