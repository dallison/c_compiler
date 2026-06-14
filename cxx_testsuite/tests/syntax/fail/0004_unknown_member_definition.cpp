// RUN: -std=c++17
class Defined {
public:
  int get(void);
};

int Defined::missing(void) {
  return 0;
}
