// RUN: -std=c++17
namespace ns {
  class Defined {
  public:
    Defined(void);
    ~Defined(void);
  };
}

ns::Defined::Defined(void) {
}

ns::Defined::~Defined(void) {
}

int main(void) {
  return 0;
}
