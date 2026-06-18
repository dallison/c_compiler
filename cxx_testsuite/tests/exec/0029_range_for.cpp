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

int main(void) {
  int values[4];
  values[0] = 1;
  values[1] = 2;
  values[2] = 3;
  values[3] = 4;

  int sum = 0;
  for (auto value : values) {
    sum += value;
  }
  if (sum != 10) {
    return 1;
  }

  for (int& value : values) {
    value += 10;
  }
  if (values[0] != 11 || values[1] != 12 ||
      values[2] != 13 || values[3] != 14) {
    return 2;
  }

  int count = 0;
  for (int value : values) {
    count += value > 12;
  }
  if (count != 2) {
    return 3;
  }

  IntRange ints;
  ints.first = values;
  ints.last = values + 4;
  int iter_sum = 0;
  for (int value : ints) {
    iter_sum += value;
  }
  if (iter_sum != 50) {
    return 4;
  }

  Pair pairs[2];
  pairs[0].x = 1;
  pairs[0].y = 2;
  pairs[1].x = 3;
  pairs[1].y = 4;
  int pair_sum = 0;
  for ([x, y] : pairs) {
    pair_sum += x * 10 + y;
  }
  if (pair_sum != 46) {
    return 5;
  }

  PairRange pair_range;
  pair_range.first = pairs;
  pair_range.last = pairs + 2;
  int range_pair_sum = 0;
  for ([x, y] : pair_range) {
    range_pair_sum += x + y;
  }
  if (range_pair_sum != 10) {
    return 6;
  }
  return 0;
}
