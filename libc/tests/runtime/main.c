#include "test_framework.h"

int TestAbs(void);
int TestStrtol(void);
int TestSetjmp(void);
int TestEHFrame(void);
int TestLSDAParser(void);
int TestEHCxa(void);
int TestEHItaniumRuntime(void);

int main(void) {
  int result;
  if ((result = TestAbs()) != 0) return result;
  if ((result = TestStrtol()) != 0) return result;
  if ((result = TestSetjmp()) != 0) return result;
  if ((result = TestEHFrame()) != 0) return result;
  if ((result = TestLSDAParser()) != 0) return result;
  if ((result = TestEHCxa()) != 0) return result;
  return TestEHItaniumRuntime();
}
