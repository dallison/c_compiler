// RUN: -std=c++17
// EXPECT: Ambiguous overload for Box
struct Box {
  Box(short v);
  Box(long v);
};

int main(void) {
  char c;
  Box box(c);
  return 0;
}
