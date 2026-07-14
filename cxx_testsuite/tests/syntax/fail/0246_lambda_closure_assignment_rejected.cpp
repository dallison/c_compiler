// RUN: -std=c++20 -ferror
int main(void) {
  int x = 1;
  auto first = [x] {
    return x;
  };
  auto second = [x] {
    return x + 1;
  };
  first = second;
  return first();
}
