// RUN: -std=c++17
class Counter {
public:
  int value;
  int get(void);
  int add(int delta);
};

int Counter::get(void) {
  return value;
}

int Counter::add(int delta) {
  return value + delta;
}

int main(void) {
  Counter counter;
  Counter *ptr;
  return sizeof(counter.get()) + sizeof(ptr->get()) + sizeof(counter.add(1));
}
