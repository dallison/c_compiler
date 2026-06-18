// RUN: -std=c++20
struct IntRange {
  int* first;
  int* last;
  int* begin();
  int* end();
};

int* IntRange::begin() {
  return first;
}

int* IntRange::end() {
  return last;
}

struct Pair {
  int x;
  int y;
};

struct PairRange {
  Pair* first;
  Pair* last;
  Pair* begin();
  Pair* end();
};

Pair* PairRange::begin() {
  return first;
}

Pair* PairRange::end() {
  return last;
}

int sum_values(void) {
  int values[3];
  values[0] = 1;
  values[1] = 2;
  values[2] = 3;

  int sum = 0;
  for (auto value : values) {
    sum += value;
  }
  for (int& value : values) {
    value += 1;
  }

  IntRange range;
  range.first = values;
  range.last = values + 3;
  for (int value : range) {
    sum += value;
  }

  Pair pairs[2];
  pairs[0].x = 1;
  pairs[0].y = 2;
  pairs[1].x = 3;
  pairs[1].y = 4;
  for ([x, y] : pairs) {
    sum += x + y;
  }

  PairRange pair_range;
  pair_range.first = pairs;
  pair_range.last = pairs + 2;
  for ([x, y] : pair_range) {
    sum += x * y;
  }
  return sum + values[0] + values[1] + values[2];
}
