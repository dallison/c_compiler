// RUN: -std=c++20

struct A {
  int value;
};

struct B {
  int value;
};

B b = {23};

int main(void) {
  try {
    throw b;
    return 1;
  } catch (A value) {
    return 2;
  } catch (B value) {
    return value.value - 23;
  }
}
