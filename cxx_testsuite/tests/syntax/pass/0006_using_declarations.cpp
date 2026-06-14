// RUN: -std=c++17
namespace source {
  int value;
  typedef int Number;
}

using source::value;
using source::Number;

Number global_number;

int main(void) {
  return value + global_number;
}
