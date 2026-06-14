// RUN: -std=c++17
class WithStatic {
public:
  static int count;
  static int get_count(void);
private:
  int value;
};

WithStatic with_static;

int main(void) {
  return 0;
}
