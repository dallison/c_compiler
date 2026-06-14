// RUN: -std=c++17
using Number = int;
using NumberPtr = int *;

Number global_number;
NumberPtr global_pointer;

int main(void) {
  using LocalNumber = Number;
  LocalNumber local = 0;
  return local + global_number;
}
