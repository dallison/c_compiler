// RUN: -std=c++17
namespace ns {
  class Defined {
  public:
    int get(void);
  };
}

int ns::Defined::get(void) {
  return 0;
}

int main(void) {
  return 0;
}
