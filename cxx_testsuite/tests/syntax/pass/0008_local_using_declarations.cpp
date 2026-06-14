// RUN: -std=c++17
namespace local_source {
  int value;
  typedef int Number;
}

int main(void) {
  using local_source::value;
  using local_source::Number;
  Number number = 0;
  return value + number;
}
