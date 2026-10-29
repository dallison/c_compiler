// RUN: -std=c++20
// EXPECT: 'y' is not captured
// EXPECT: 'local' is not captured

int generic(int y) {
  auto lambda = [](auto&&...) { return y; };
  return lambda();
}

int plain(int z) {
  int local = z;
  auto lambda = [] { return local; };
  return lambda();
}

int main() {
  return generic(4) + plain(5);
}
