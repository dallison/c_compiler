// RUN: -std=c++20

struct PCodeMemberBox {
  int value;

  constexpr int read(void) const {
    return value;
  }

  constexpr int add(int amount) const {
    return value + amount;
  }
};

struct PCodePoint {
  int x;
  int y;

  constexpr PCodePoint(int x_value, int y_value) : x(x_value), y(y_value) {}

  constexpr int sum(void) const {
    return x + y;
  }
};

struct PCodePointHolder {
  PCodePoint point;
  int extra;

  constexpr int total(void) const {
    return point.sum() + extra;
  }
};

struct PCodeCluster {
  PCodePoint points[3];
  int bias;

  constexpr int total(void) const {
    int result = bias;
    for (int i = 0; i < 3; ++i) {
      result += points[i].sum();
    }
    return result;
  }
};

constexpr PCodeMemberBox pcode_member_box = {37};
constexpr PCodePoint pcode_constructed_point(19, 23);
constexpr PCodePointHolder pcode_constructed_holder = {
    PCodePoint(10, 20),
    12,
};

constexpr PCodeMemberBox pcode_member_factory(int value) {
  PCodeMemberBox box = {value};
  return box;
}

constexpr PCodeMemberBox pcode_member_factory_box =
    pcode_member_factory(40);

constexpr int pcode_pointer_reference_values(void) {
  int values[2] = {4, 5};
  int* ptr = &values[0];
  int& ref = values[1];
  ref += *ptr;
  *ptr += 2;
  return values[0] + values[1];
}

constexpr int pcode_class_array_values(void) {
  PCodePoint points[2] = {PCodePoint(5, 6), PCodePoint(7, 8)};
  return points[0].sum() + points[1].x + points[1].y;
}

constexpr int pcode_mutate_class_array(void) {
  PCodePoint points[2] = {PCodePoint(1, 2), PCodePoint(3, 4)};
  points[0].x += points[1].y;
  points[1].y += points[0].sum();
  return points[0].sum() + points[1].sum();
}

constexpr PCodePoint pcode_shift_point(PCodePoint point, int dx, int dy) {
  point.x += dx;
  point.y += dy;
  return point;
}

constexpr PCodeCluster pcode_adjust_cluster(PCodeCluster cluster, int delta) {
  for (int i = 0; i < 3; ++i) {
    cluster.points[i].x += delta;
    cluster.points[i].y -= i;
  }
  cluster.bias += cluster.points[2].x;
  return cluster;
}

constexpr int pcode_loop_mutate_cluster(void) {
  PCodeCluster cluster = {
      {PCodePoint(1, 2), PCodePoint(3, 4), PCodePoint(5, 6)},
      7,
  };
  for (int i = 0; i < 3; ++i) {
    cluster.points[i].x += cluster.bias + i;
    cluster.points[i].y += cluster.points[0].x;
  }
  return cluster.total();
}

constexpr int pcode_by_value_cluster_calls(void) {
  PCodeCluster original = {
      {PCodePoint(2, 3), PCodePoint(4, 5), PCodePoint(6, 7)},
      1,
  };
  PCodeCluster adjusted = pcode_adjust_cluster(original, 5);
  original.points[0].x = 20;
  return original.total() + adjusted.total();
}

constexpr int pcode_chained_call_results(void) {
  PCodePoint source_left(1, 2);
  PCodePoint source_right(5, 6);
  PCodePoint left = pcode_shift_point(source_left, 3, 4);
  PCodePoint right = pcode_shift_point(source_right, 7, 8);
  return left.sum() * 2 + right.sum();
}

constexpr int pcode_conditional_cluster_walk(int seed) {
  PCodeCluster cluster = {
      {PCodePoint(seed, seed + 1), PCodePoint(seed + 2, seed + 3),
       PCodePoint(seed + 4, seed + 5)},
      seed,
  };
  int result = 0;
  for (int i = 0; i < 3; ++i) {
    if ((cluster.points[i].sum() & 1) == 0) {
      result += pcode_shift_point(cluster.points[i], i, seed).sum();
    } else {
      result -= cluster.points[i].x;
    }
  }
  return result + cluster.bias;
}

static_assert(pcode_member_box.read() == 37,
              "pcode constexpr const member call reads this");
static_assert(pcode_member_box.add(5) == 42,
              "pcode constexpr const member call with argument");
static_assert(pcode_member_factory_box.add(2) == 42,
              "pcode constexpr aggregate return initializes object");
static_assert(pcode_pointer_reference_values() == 15,
              "pcode constexpr pointer and reference values");
static_assert(pcode_constructed_point.sum() == 42,
              "pcode constexpr constructor initializes object");
static_assert(pcode_constructed_holder.total() == 42,
              "pcode constexpr nested constructor initializes aggregate slot");
static_assert(pcode_class_array_values() == 26,
              "pcode constexpr class array values");
static_assert(pcode_mutate_class_array() == 21,
              "pcode constexpr class array mutation");
static_assert(pcode_loop_mutate_cluster() == 76,
              "pcode constexpr loop mutates array-bearing aggregate");
static_assert(pcode_by_value_cluster_calls() == 97,
              "pcode constexpr aggregate array by-value calls");
static_assert(pcode_chained_call_results() == 46,
              "pcode constexpr object-return call results");
static_assert(pcode_conditional_cluster_walk(2) == -10,
              "pcode constexpr conditionals over aggregate arrays");
