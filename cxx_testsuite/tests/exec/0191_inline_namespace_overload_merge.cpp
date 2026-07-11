// RUN: -std=c++20
namespace direct_inline {
int pick(int x) {
  return x + 1;
}

inline namespace v1 {
int pick(double x) {
  return (int)(x + 2.0);
}
}  // namespace v1
}  // namespace direct_inline

namespace sibling_inline {
inline namespace a {
int compute(int x) {
  return x + 10;
}
}  // namespace a

inline namespace b {
int compute(double x) {
  return (int)(x + 20.0);
}
}  // namespace b
}  // namespace sibling_inline

int global_pick(int x) {
  return x + 30;
}

inline namespace global_v1 {
int global_pick(double x) {
  return (int)(x + 40.0);
}
}  // namespace global_v1

int test_direct_inline(void) {
  if (direct_inline::pick(3) != 4) {
    return 1;
  }
  if (direct_inline::pick(3.0) != 5) {
    return 2;
  }
  return 0;
}

int test_sibling_inline(void) {
  if (sibling_inline::compute(1) != 11) {
    return 3;
  }
  if (sibling_inline::compute(1.0) != 21) {
    return 4;
  }
  return 0;
}

int test_global_inline(void) {
  if (global_pick(1) != 31) {
    return 5;
  }
  if (global_pick(1.0) != 41) {
    return 6;
  }
  return 0;
}

int main(void) {
  if (test_direct_inline() != 0) {
    return test_direct_inline();
  }
  if (test_sibling_inline() != 0) {
    return 10 + test_sibling_inline();
  }
  if (test_global_inline() != 0) {
    return 20 + test_global_inline();
  }
  return 0;
}
