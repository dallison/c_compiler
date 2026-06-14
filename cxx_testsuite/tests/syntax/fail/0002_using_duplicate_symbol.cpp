// RUN: -std=c++17
namespace source {
  int value;
}

int value;
using source::value;

int main(void) {
  return 0;
}
