// RUN: -std=c++17
typedef int GlobalInt;

namespace types {
  typedef int NamespaceInt;

  namespace nested {
    typedef int NestedInt;
  }
}

types::NamespaceInt namespace_value;
types::nested::NestedInt nested_value;

int main(void) {
  ::GlobalInt local = 0;
  return local + namespace_value + nested_value;
}
