// RUN: -std=c++17
class Lifecycle {
public:
  Lifecycle();
  ~Lifecycle();
private:
  int value;
};

Lifecycle lifecycle;

int main(void) {
  return 0;
}
