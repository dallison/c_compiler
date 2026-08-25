// RUN: -std=c++20
// EXPECT: Structured binding declaration requires an initializer
// A declaration with brackets before its name can be recovered as an empty
// structured binding. Semantic analysis must tolerate its missing initializer.
void test() {
  int []value = {0};
}
