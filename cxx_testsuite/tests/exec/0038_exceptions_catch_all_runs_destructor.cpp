// RUN: -std=c++20

int destroyed = 1;

struct Tracker {
  int value;

  Tracker() : value(7) {
  }

  ~Tracker();
};

Tracker::~Tracker() {
  destroyed = 0;
}

int main(void) {
  try {
    Tracker tracker;
    throw 42;
    return 2;
  } catch (...) {
    return destroyed;
  }
}
