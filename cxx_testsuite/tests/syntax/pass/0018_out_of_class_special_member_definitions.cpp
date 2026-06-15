// RUN: -std=c++17
class Defined {
public:
  Defined(void);
  ~Defined(void);
};

Defined::Defined(void) {
}

Defined::~Defined(void) {
}

int main(void) {
  return 0;
}
