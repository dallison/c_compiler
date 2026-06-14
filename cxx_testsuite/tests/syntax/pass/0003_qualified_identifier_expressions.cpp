// RUN: -std=c++17
int global_value;

namespace outer {
  int namespace_value;

  namespace inner {
    int nested_value;

    int use_qualified_values(void) {
      return ::global_value + outer::namespace_value + outer::inner::nested_value;
    }
  }
}

int main(void) {
  return outer::inner::use_qualified_values();
}
