// RUN: -std=c++20

namespace LeftNS {
struct Item {
  int value;
};

Item make_item(void) {
  Item item = {11};
  return item;
}
}  // namespace LeftNS

namespace RightNS {
struct Item {
  int value;
};

Item make_item(void) {
  Item item = {17};
  return item;
}
}  // namespace RightNS

struct NonAggregateBox {
  int values[2];
  NonAggregateBox(int a, int b);
};

NonAggregateBox::NonAggregateBox(int a, int b) {
  values[0] = a;
  values[1] = b;
}

int main(void) {
  LeftNS::Item left = LeftNS::make_item();
  RightNS::Item right = RightNS::make_item();
  if (left.value != 11 || right.value != 17) {
    return 1;
  }

  NonAggregateBox box(3, 4);
  NonAggregateBox copied = box;
  if (copied.values[0] != 3 || copied.values[1] != 4) {
    return 2;
  }

  return 0;
}
