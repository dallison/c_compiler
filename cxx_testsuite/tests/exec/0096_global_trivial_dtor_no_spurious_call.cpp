// A class with a user-declared constructor but only a trivial (implicit)
// destructor must not cause a spurious destructor call for a global/static
// instance to be injected into main.  Previously that injection clobbered
// main's body, producing a wrong result or a crash.
// EXPECT_EXIT: 42
int putchar(int c) asm("putchar");

struct Box {
  int v;
  constexpr explicit Box(int x) : v(x) {}
  constexpr bool negative() const { return v < 0; }
};

static constexpr Box minus = {(signed char)-1};
static constexpr Box plus = {7};

int main(void) {
  putchar('M');
  putchar('\n');
  if (!minus.negative()) {
    return 1;
  }
  if (plus.negative()) {
    return 2;
  }
  return plus.v + (minus.negative() ? 35 : 0);
}
