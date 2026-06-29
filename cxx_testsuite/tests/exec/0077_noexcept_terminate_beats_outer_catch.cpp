// RUN: -std=c++20
// EXPECT_EXIT: nonzero

// The defining conformance case: std::terminate is called even though an
// enclosing try/catch could match the thrown type.  Because the exception
// cannot escape the noexcept function, the outer handler is never reached.
void boom(void) noexcept {
  throw 42;
}

int main(void) {
  try {
    boom();
  } catch (...) {
    return 0;  // Must NOT run: terminate happens first.
  }
  return 1;
}
