// RUN: -std=c++20
// Empty init-statements and empty controlled statements are represented by
// null AST children and must remain valid through parsing and analysis.
struct Range {
  int* begin();
  int* end();
};

void test(int value) {
  if (; true)
    ;
  switch (; value) {
  }
  for (; int item : Range())
    ;
}
