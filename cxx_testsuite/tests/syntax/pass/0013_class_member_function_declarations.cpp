// RUN: -std=c++17
class Methods {
public:
  int value;
  int get(void);
  void set(int value);
  int inline_value(void) {
    return 0;
  }
};

Methods methods;

int main(void) {
  return 0;
}
