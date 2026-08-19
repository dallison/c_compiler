// RUN: -std=c++29
// EXPECT: must produce exactly one valid C++ identifier

void invalid_id_keyword() {
  (void)^{ \id("int") };
}
