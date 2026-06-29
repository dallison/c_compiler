// RUN: -std=c++20
// EXPECT_EXIT: 0

// A noexcept function may contain a try/catch.  An exception caught locally
// never escapes, so std::terminate is not called and execution continues
// normally.
int observed = 7;

void safe(void) noexcept {
  try {
    throw 1;
  } catch (...) {
    observed = 0;
  }
}

int main(void) {
  safe();
  return observed;  // 0 when the local handler ran.
}
