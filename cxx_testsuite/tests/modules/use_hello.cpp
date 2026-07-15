import hello;

int main() {
  if (module_answer() != 42) {
    return 1;
  }
  if (greet::value != 7) {
    return 2;
  }
  return 0;
}
