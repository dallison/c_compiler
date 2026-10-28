// RUN: -std=c++20
// EXPECT_EXIT: 0

int flag;
int swallowed;

struct Boom {
  Boom() { throw 1; }
};

struct S {
  Boom b;
  S() try : b() {
  } catch (int) {
    flag = 1;
  }
};

void f() try {
  throw 1;
} catch (int) {
  swallowed = 3;
}

int a<:2:> = <% 4, 5 %>;

int main() {
  int caught = 0;
  try {
    S s;
  } catch (int) {
    caught = 1;
  }
  if (!caught || flag != 1) {
    return 1;
  }
  f();
  if (swallowed != 3) {
    return 2;
  }
  if (a<:0:> != 4 || a<:1:> != 5) {
    return 3;
  }
  return 0;
}
