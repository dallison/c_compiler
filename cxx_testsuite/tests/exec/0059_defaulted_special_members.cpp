// RUN: -std=c++20

struct DefaultedRuntimeBox {
  int left;
  int right;
  int values[3];

  DefaultedRuntimeBox(int l, int r, int a, int b, int c) {
    left = l;
    right = r;
    values[0] = a;
    values[1] = b;
    values[2] = c;
  }
  DefaultedRuntimeBox(const DefaultedRuntimeBox& other) = default;
  DefaultedRuntimeBox(DefaultedRuntimeBox&& other) = default;
  struct DefaultedRuntimeBox& operator=(
      const DefaultedRuntimeBox& other) = default;
  struct DefaultedRuntimeBox& operator=(DefaultedRuntimeBox&& other) = default;
};

int sum(DefaultedRuntimeBox& box) {
  return box.left + box.right + box.values[0] + box.values[1] + box.values[2];
}

int main(void) {
  DefaultedRuntimeBox first(1, 2, 3, 4, 5);
  DefaultedRuntimeBox second(first);
  if (sum(second) != 15) {
    return 1;
  }

  DefaultedRuntimeBox third(10, 20, 30, 40, 50);
  third = first;
  if (sum(third) != 15) {
    return 2;
  }

  DefaultedRuntimeBox fourth((DefaultedRuntimeBox&&)first);
  if (sum(fourth) != 15) {
    return 3;
  }

  DefaultedRuntimeBox fifth(6, 7, 8, 9, 10);
  fifth = (DefaultedRuntimeBox&&)third;
  if (sum(fifth) != 15) {
    return 4;
  }

  return 0;
}
