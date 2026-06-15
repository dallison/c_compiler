// RUN: -std=c++17
// EXPECT: Ambiguous overload for pick
struct Box {
  int pick(short v);
  int pick(long v);
};

int main(void) {
  Box box;
  char c;
  return sizeof(box.pick(c));
}
