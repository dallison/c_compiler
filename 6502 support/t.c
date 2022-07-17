int factorial(int n) {
  if (n == 1) {
    return 1;
  }
  return n * factorial(n-1);
}

int main(int argc, char** argv) {
  for (int i = 0; i < 10; i++) {
    factorial(i);
  }
}
