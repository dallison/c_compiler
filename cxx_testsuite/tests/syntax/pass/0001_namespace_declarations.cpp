// RUN: -std=c++11
namespace alpha {
  int namespace_value;

  int namespace_function(void) {
    return 0;
  }
}

namespace alpha {
  extern int namespace_value;
}

int main(void) {
  return 0;
}
