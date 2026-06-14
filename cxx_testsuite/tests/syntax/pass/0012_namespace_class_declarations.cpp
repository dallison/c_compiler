// RUN: -std=c++17
namespace ns {
  class Namespaced {
    int value;
  };
}

ns::Namespaced namespaced_value;
class ns::Namespaced* namespaced_pointer;

int main(void) {
  return 0;
}
