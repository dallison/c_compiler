// RUN: -std=c++20
// EXPECT: Ambiguous overload for ambiguous_default
int ambiguous_default(int value, int extra = 0);
int ambiguous_default(int value);

int use_ambiguous_default(void) {
  return ambiguous_default(1);
}
