// RUN: -std=c++23
// EXPECT: static lambda cannot have captures

int main() {
  int value = 1;
  auto function = [value] static { return value; };
  return function();
}
