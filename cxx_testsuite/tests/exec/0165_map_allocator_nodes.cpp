// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <map>

struct Tracker {
  static int live;
  int value;

  Tracker() : value(0) { live++; }
  Tracker(int v) : value(v) { live++; }
  Tracker(const Tracker& other) : value(other.value) { live++; }
  ~Tracker() { live--; }

  Tracker& operator=(const Tracker& other) {
    value = other.value;
    return *this;
  }
};

int Tracker::live = 0;

int main() {
  {
    std::map<int, Tracker> values;
    values.insert(std::pair<const int, Tracker>(1, Tracker(7)));
    values[2] = Tracker(9);
    if (values.size() != 2 || values.find(1)->second.value != 7 ||
        values.find(2)->second.value != 9) {
      return 1;
    }
    std::map<int, Tracker> copy = values;
    if (copy.size() != 2 || copy.find(1)->second.value != 7 ||
        copy.find(2)->second.value != 9) {
      return 2;
    }
    values.clear();
    if (!values.empty() || copy.size() != 2) {
      return 3;
    }
  }
  return Tracker::live == 0 ? 0 : 4;
}
