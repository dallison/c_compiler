// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <initializer_list>
#include <variant>
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

struct BigAligned {
  alignas(16) char data;
};

int main(void) {
  std::variant<int, const char*> value;
  if (value.index() != 0 || std::get<int>(value) != 0) {
    return 1;
  }

  std::variant<int, const char*> text("hello");
  if (text.index() != 1 || std::get<1>(text)[1] != 'e') {
    return 2;
  }

  value = 17;
  if (!std::holds_alternative<int>(value) || std::get<0>(value) != 17) {
    return 3;
  }

  value = "world";
  if (!std::holds_alternative<const char*>(value) ||
      std::get<const char*>(value)[0] != 'w') {
    return 4;
  }

  value.emplace<0>(23);
  if (value.index() != 0 || std::get_if<0>(&value) == nullptr ||
      *std::get_if<int>(&value) != 23) {
    return 5;
  }

  std::variant<int, InitListTracker> list_value(std::in_place_index<1>,
                                                {1, 2, 3}, 4);
  if (list_value.index() != 1 || std::get<1>(list_value).sum != 6 ||
      std::get<InitListTracker>(list_value).extra != 4) {
    return 6;
  }

  list_value.emplace<InitListTracker>({8, 9}, 10);
  if (std::get<1>(list_value).sum != 17 ||
      std::get<1>(list_value).extra != 10) {
    return 7;
  }

  std::variant<int, const char*> left(1);
  std::variant<int, const char*> right("x");
  std::swap(left, right);
  if (left.index() != 1 || right.index() != 0 || std::get<0>(right) != 1) {
    return 8;
  }

  {
    std::variant<int, Tracker> tracked(std::in_place_type<Tracker>, 30);
    if (tracked.index() != 1 || std::get<Tracker>(tracked).value != 30 ||
        live_count != 1) {
      return 9;
    }

    std::variant<int, Tracker> copied(tracked);
    if (std::get<Tracker>(copied).value != 30 || live_count != 2 ||
        copy_count == 0) {
      return 10;
    }

    std::variant<int, Tracker> moved(std::move(copied));
    if (std::get<Tracker>(moved).value != 30 || live_count != 3 ||
        move_count == 0) {
      return 11;
    }

    tracked = 99;
    if (tracked.index() != 0 || std::get<int>(tracked) != 99 ||
        live_count != 2) {
      return 12;
    }
  }

  if (live_count != 0) {
    return 13;
  }

  if (alignof(std::variant<BigAligned, int>) < alignof(BigAligned)) {
    return 14;
  }

  return 0;
}
