struct Box {
  Box();
  ~Box();
  int value;
};

Box::Box() {
  value = 7;
}

Box::~Box() {
}

int main(void) {
  int* scalar = new int(41);
  if (*scalar != 41) {
    return 1;
  }
  delete scalar;

  Box* box = new Box;
  if (box->value != 7) {
    return 2;
  }
  delete box;

  Box* boxes = new Box[3];
  if (boxes[0].value != 7 || boxes[1].value != 7 ||
      boxes[2].value != 7) {
    return 4;
  }
  delete[] boxes;

  int* values = new int[2];
  values[0] = 3;
  values[1] = 4;
  int sum = values[0] + values[1];
  delete[] values;
  if (sum != 7) {
    return 6;
  }

  return 0;
}
