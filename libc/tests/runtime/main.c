#include "test_framework.h"

int TestAbs(void);
int TestStrtol(void);
int TestSetjmp(void);
int TestEHFrame(void);

int main(void) {
  return TestAbs() + TestStrtol() + TestSetjmp() + TestEHFrame();
}
