#include "test_framework.h"

int TestAbs(void);
int TestSetjmp(void);
int TestEHFrame(void);

int main(void) {
  return TestAbs() + TestSetjmp() + TestEHFrame();
}
