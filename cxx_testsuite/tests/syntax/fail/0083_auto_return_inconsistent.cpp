// RUN: -std=c++20
// EXPECT: Inconsistent auto function return type

auto inconsistent_auto_return(int which) {
  if (which) {
    return 1;
  }
  return 2L;
}
