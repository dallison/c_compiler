// RUN: -std=c++20

struct Abstract {
  virtual int value(void) = 0;
};

int main(void) {
  Abstract* object = new Abstract;
  return object == 0;
}
