#include "cxx_demangle.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

static void Expect(const char* mangled, const char* expected) {
  String out;
  StringInit(&out, NULL);
  bool ok = CXXDemangle(mangled, &out);
  if (!ok || strcmp(out.value, expected) != 0) {
    fprintf(stderr, "FAIL %s\n  got:  %s\n  want: %s\n", mangled,
            ok ? out.value : "(demangle failed)", expected);
    failures++;
  }
  StringDestruct(&out);
}

static void ExpectFail(const char* name) {
  String out;
  StringInit(&out, NULL);
  if (CXXDemangle(name, &out)) {
    fprintf(stderr, "FAIL expected no demangle for %s, got %s\n", name,
            out.value);
    failures++;
  }
  StringDestruct(&out);
}

int main(void) {
  Expect("_ZN2ns1fEi", "ns::f(int)");
  Expect("_ZN2ns1fEj", "ns::f(unsigned int)");
  Expect("_ZN3BoxC1Ev", "Box::<constructor>()");
  Expect("_ZN3BoxD1Ev", "Box::<destructor>()");
  Expect("_ZN3Box6methodEi", "Box::method(int)");
  Expect("_ZNK3Box5valueEv", "Box::value() const");
  Expect("_ZN3Box4statEd", "Box::stat(double)");
  Expect("_Zpl3BoxS_", "operator+(Box, Box)");
  Expect("_ZN4Game4quitEv", "Game::quit()");
  Expect("_ZN4Game11show_cannonEPKh",
         "Game::show_cannon(unsigned char const*)");
  Expect("_ZW12alpha_mangleE6secretv", "alpha_mangle::secret()");
  ExpectFail("main");
  ExpectFail("_foo");
  if (failures != 0) {
    fprintf(stderr, "%d demangle test(s) failed\n", failures);
    return 1;
  }
  return 0;
}
