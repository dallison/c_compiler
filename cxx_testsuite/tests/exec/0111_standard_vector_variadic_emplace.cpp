// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <vector>
#include <utility>

struct Source {
  int value;

  explicit Source(int v) : value(v) {
  }
};

struct Emplaced {
  int first;
  int second;
  int category;

  static int live;
  static int copied;
  static int moved;

  Emplaced() : first(0), second(0), category(0) {
    ++live;
  }

  Emplaced(int a, int b, int c) : first(a + b), second(c), category(3) {
    ++live;
  }

  Emplaced(Source& source, int extra)
      : first(source.value), second(extra), category(1) {
    ++live;
  }

  Emplaced(Source&& source, int extra)
      : first(source.value), second(extra), category(2) {
    source.value = -1;
    ++live;
  }

  Emplaced(const Emplaced& other)
      : first(other.first), second(other.second), category(other.category) {
    ++live;
    ++copied;
  }

  Emplaced(Emplaced&& other)
      : first(other.first), second(other.second), category(other.category) {
    other.first = -1;
    ++live;
    ++moved;
  }

  Emplaced& operator=(Emplaced&& other) {
    first = other.first;
    second = other.second;
    category = other.category;
    other.first = -1;
    ++moved;
    return *this;
  }

  ~Emplaced() {
    --live;
  }
};

int Emplaced::live = 0;
int Emplaced::copied = 0;
int Emplaced::moved = 0;

int main(void) {
  {
    std::vector<Emplaced> values;
    Emplaced& first = values.emplace_back(1, 2, 3);
    if (values.size() != 1 || first.first != 3 || first.second != 3 ||
        first.category != 3) {
      return 1;
    }

    Source lvalue(10);
    values.emplace_back(lvalue, 4);
    if (values.size() != 2 || values[1].first != 10 ||
        values[1].second != 4 || values[1].category != 1 ||
        lvalue.value != 10) {
      return 2;
    }

    Source rvalue(20);
    values.emplace_back(std::move(rvalue), 5);
    if (values.size() != 3 || values[2].first != 20 ||
        values[2].second != 5 || values[2].category != 2 ||
        rvalue.value != -1) {
      return 3;
    }

    int moves_before_middle_emplace = Emplaced::moved;
    values.emplace(values.begin() + 1, 7, 8, 9);
    if (values.size() != 4 || values[1].first != 15 ||
        values[1].second != 9 || values[1].category != 3) {
      return 4;
    }
    if (Emplaced::moved != moves_before_middle_emplace) {
      return 8;
    }

    Source middle(30);
    moves_before_middle_emplace = Emplaced::moved;
    values.emplace(values.begin() + 2, std::move(middle), 6);
    if (values.size() != 5 || values[2].first != 30 ||
        values[2].second != 6 || values[2].category != 2 ||
        middle.value != -1) {
      return 5;
    }
    if (Emplaced::moved != moves_before_middle_emplace) {
      return 9;
    }
  }

  if (Emplaced::live != 0) {
    return 6;
  }
  if (Emplaced::copied == 0 || Emplaced::moved == 0) {
    return 7;
  }
  return 0;
}
