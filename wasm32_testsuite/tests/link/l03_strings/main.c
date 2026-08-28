// String literals in both objects, plus a pointer to one crossing between
// them.  Two objects each numbering their literals from zero is the case
// that catches a link that keeps compiler-local ids.

extern const char* greeting;
int length(const char* s);
int first(void);

int main(void) {
  const char* local = "abcdefg";
  return length(local) + length(greeting) + first();
}
