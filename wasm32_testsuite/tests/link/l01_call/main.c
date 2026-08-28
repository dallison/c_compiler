// Calls that leave the translation unit, in both directions.

int add(int a, int b);
int scale(int a);
int callback(int a) { return a * 3; }

int main(void) {
  int total = add(4, 5);
  total += scale(6);
  return total;
}
