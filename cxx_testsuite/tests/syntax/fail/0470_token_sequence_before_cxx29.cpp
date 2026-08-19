// RUN: -std=c++26
// EXPECT: token sequence literals require C++29

void before_cxx29() {
  (void)^{ a + b };
}
