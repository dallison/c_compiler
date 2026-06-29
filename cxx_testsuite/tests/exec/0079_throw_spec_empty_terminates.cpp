// RUN: -std=c++20
// EXPECT_EXIT: nonzero

// The deprecated empty exception specification throw() is equivalent to
// noexcept, so an escaping throw likewise calls std::terminate.
void boom(void) throw() {
  throw 42;
}

int main(void) {
  boom();
  return 0;
}
