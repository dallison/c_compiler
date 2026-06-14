// RUN: -std=c++17
class Defined {
public:
  int get(void);
  static int static_get(void);
};

int Defined::get(void) {
  return 0;
}

int Defined::static_get(void) {
  return 0;
}

int main(void) {
  return 0;
}
