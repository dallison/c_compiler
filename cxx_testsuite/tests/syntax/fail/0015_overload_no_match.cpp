// RUN: -std=c++17
// EXPECT: No matching overload for pick
int pick(int *value) {
  return sizeof(value);
}

int pick(char *value) {
  return sizeof(value);
}

int main(void) {
  return sizeof(pick(1));
}
