// RUN: -std=c++11
// EXPECT: primary expression expected
// A colon following a number is not a valid label. Recovery must consume the
// colon instead of repeatedly trying to parse it as another statement.
void test(int value) {
  switch (value) {
    2: return;
  }
}
