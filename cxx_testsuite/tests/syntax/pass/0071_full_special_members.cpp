// RUN: -std=c++20

struct AggregatePair {
  int first;
  int second;
};

AggregatePair aggregate_pair = {1, 2};

struct NonAggregateBox {
  int value;
  NonAggregateBox(int v);
};

NonAggregateBox::NonAggregateBox(int v) {
  value = v;
}

struct OutOfClassDefaulted {
  int values[2];
  OutOfClassDefaulted(int a, int b);
  OutOfClassDefaulted(const struct OutOfClassDefaulted& other);
};

OutOfClassDefaulted::OutOfClassDefaulted(int a, int b) {
  values[0] = a;
  values[1] = b;
}

OutOfClassDefaulted::OutOfClassDefaulted(
    const struct OutOfClassDefaulted& other) = default;

void use_full_special_members() {
  NonAggregateBox box(3);
  NonAggregateBox copied = box;
  OutOfClassDefaulted source(4, 5);
  OutOfClassDefaulted target(source);
}
