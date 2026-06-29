// RUN: -std=c++20
// EXPECT_EXIT: nonzero

// An exception that escapes a noexcept function must call std::terminate
// (which aborts), per [except.spec].
void boom(void) noexcept {
  throw 42;
}

int main(void) {
  boom();
  return 0;
}
