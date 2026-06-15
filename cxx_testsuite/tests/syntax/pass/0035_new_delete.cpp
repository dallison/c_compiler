// RUN: -std=c++17
struct Box {
  Box();
  ~Box();
  int value;
};

Box::Box() {
}

Box::~Box() {
}

int main(void) {
  int* value = new int;
  int* initialized_value = new int(1);
  int* brace_value = new int{2};
  int* values = new int[3];
  Box* box = new Box;
  Box* boxes = new Box[2];
  delete value;
  delete initialized_value;
  delete brace_value;
  delete[] values;
  delete box;
  delete[] boxes;
  return 0;
}
