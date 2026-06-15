// RUN: -std=c++17
class Counter {
public:
  int add(int delta);
};

int Counter::add(int delta) {
  return delta;
}

int main(void) {
  Counter counter;
  return sizeof(counter.add());
}
