// RUN: -std=c++17
namespace imported {
  int value;
  typedef int Number;
}

using namespace imported;

Number global_number;

int main(void) {
  return value + global_number;
}
