// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <optional>
#include <utility>

int live_count = 0;
int copy_count = 0;
int move_count = 0;

struct Tracker {
  int value;

  explicit Tracker(int v) : value(v) {
    ++live_count;
  }

  Tracker(const Tracker& other) : value(other.value) {
    ++live_count;
    ++copy_count;
  }

  Tracker(Tracker&& other) : value(other.value) {
    ++live_count;
    ++move_count;
    other.value = -1;
  }

  Tracker& operator=(const Tracker& other) {
    value = other.value;
    ++copy_count;
    return *this;
  }

  Tracker& operator=(Tracker&& other) {
    value = other.value;
    ++move_count;
    other.value = -1;
    return *this;
  }

  ~Tracker() {
    --live_count;
  }
};

bool operator==(const Tracker& left, const Tracker& right) {
  return left.value == right.value;
}

bool operator<(const Tracker& left, const Tracker& right) {
  return left.value < right.value;
}

struct BigAligned {
  alignas(16) char data;
};

struct InitListTracker {
  int sum;
  int extra;

  InitListTracker(std::initializer_list<int> values, int e)
      : sum(0), extra(e) {
    for (int value : values) {
      sum += value;
    }
  }
};

int main(void) {
  std::optional<int> empty;
  if (empty.has_value() || empty != std::nullopt) {
    return 1;
  }

  std::optional<int> value(7);
  if (!value || *value != 7 || value.value() != 7) {
    return 2;
  }

  value = 9;
  if (value.value_or(3) != 9 || empty.value_or(3) != 3) {
    return 3;
  }

  value.reset();
  if (value.has_value()) {
    return 4;
  }

  value.emplace(11);
  if (*value != 11 || !(value == 11) || !(value > std::nullopt)) {
    return 5;
  }

  std::optional<int> copy(value);
  std::optional<int> moved(std::move(copy));
  if (!moved.has_value() || *moved != 11) {
    return 6;
  }

  std::optional<int> a(1);
  std::optional<int> b(2);
  std::swap(a, b);
  if (*a != 2 || *b != 1 || !(b < a)) {
    return 7;
  }

  if (!(empty < 4) || !(4 > empty) || !(value > 4) || !(4 < value) ||
      !(value >= 11) || !(11 <= value) || value < 11 || 11 > value) {
    return 8;
  }

  std::strong_ordering cmp_equal = value <=> 11;
  std::strong_ordering cmp_less = empty <=> value;
  std::strong_ordering cmp_greater = value <=> std::nullopt;
  std::strong_ordering cmp_value = a <=> b;
  if (cmp_equal != std::strong_ordering::equal ||
      cmp_less != std::strong_ordering::less ||
      cmp_greater != std::strong_ordering::greater ||
      cmp_value != std::strong_ordering::greater) {
    return 9;
  }

  bool threw = false;
  try {
    empty.value();
  } catch (const std::bad_optional_access&) {
    threw = true;
  }
  if (!threw) {
    return 10;
  }

  std::optional<int> made = std::make_optional(17);
  if (!made || *made != 17) {
    return 11;
  }

  std::optional<InitListTracker> list_value(std::in_place, {1, 2, 3}, 4);
  if (!list_value || list_value->sum != 6 || list_value->extra != 4) {
    return 12;
  }
  std::optional<InitListTracker> made_list =
      std::make_optional<InitListTracker>({8, 9}, 10);
  if (!made_list || made_list->sum != 17 || made_list->extra != 10) {
    return 13;
  }

  {
    std::optional<Tracker> tracked(std::in_place, 21);
    if (!tracked || tracked->value != 21 || live_count != 1) {
      return 14;
    }

    std::optional<Tracker> tracked_copy(tracked);
    if (tracked_copy->value != 21 || live_count != 2 || copy_count == 0) {
      return 15;
    }

    tracked.reset();
    if (tracked.has_value() || live_count != 1) {
      return 16;
    }

    tracked.emplace(34);
    if (tracked->value != 34 || live_count != 2) {
      return 17;
    }

    std::optional<Tracker> made_tracker = std::make_optional<Tracker>(55);
    if (!made_tracker || made_tracker->value != 55) {
      return 18;
    }
  }

  if (live_count != 0) {
    return 19;
  }

  if (alignof(std::optional<BigAligned>) < alignof(BigAligned)) {
    return 20;
  }

  return 0;
}
