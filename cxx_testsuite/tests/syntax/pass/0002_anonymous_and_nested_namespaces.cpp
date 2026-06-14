// RUN: -std=c++17
namespace {
  int anonymous_value;
}

namespace outer::inner {
  int nested_value;

  int nested_function(void) {
    return 0;
  }
}

int main(void) {
  return 0;
}
