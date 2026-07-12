// RUN: -std=c++20
// EXPECT: No matching overload for pick (int pick(int*))
int pick(int *value) {
  return sizeof(value);
}

int pick(char *value) {
  return sizeof(value);
}

int main(void) {
  return pick(1);
}
