// RUN: -std=c++11
// EXPECT: primary expression expected
// Editor-placeholder syntax is unsupported, but malformed placeholders around
// a case label must still make forward progress during statement recovery.
void test() {
  switch (<#expression#>) {
    case <#constant#>:
      break;
  }
}
