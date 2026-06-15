// RUN: -std=c++17
// EXPECT: Ambiguous overload for pick
int pick(short value) {
  return value;
}

int pick(long value) {
  return value;
}

int main(void) {
  char c;
  return sizeof(pick(c));
}
