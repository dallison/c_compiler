// A temporary bound to a block-scope reference lives as long as the reference
// and is destroyed when it goes out of scope ([class.temporary]/6), on every
// path out of the block.
#include <string>

static int live = 0;

struct Tracked {
  int id;
  explicit Tracked(int i) : id(i) { live++; }
  Tracked(const Tracked& other) : id(other.id) { live++; }
  ~Tracked() { live--; }
};

struct Holder {
  Tracked tracked;
  int which;
};

static Holder MakeHolder(int i) { return Holder{Tracked(i), 0}; }
static Tracked MakeTracked(int i) { return Tracked(i); }

static int ReturnThroughReference() {
  const Tracked& r = MakeTracked(3);
  return live == 1 ? r.id : -1;
}

int main() {
  {
    auto&& r = MakeHolder(1);
    if (live != 1 || r.tracked.id != 1) return 1;
  }
  if (live != 0) return 2;
  {
    const Holder& r = MakeHolder(2);
    if (live != 1 || r.tracked.id != 2) return 3;
  }
  if (live != 0) return 4;
  {
    const Tracked& r = Tracked(3);
    if (live != 1 || r.id != 3) return 5;
  }
  if (live != 0) return 6;
  {
    const auto& [tracked, which] = MakeHolder(4);
    if (live != 1 || tracked.id != 4) return 7;
  }
  if (live != 0) return 8;
  for (int i = 0; i < 3; i++) {
    const Tracked& r = MakeTracked(i);
    if (live != 1 || r.id != i) return 9;
    if (i == 1) break;
  }
  if (live != 0) return 10;
  if (ReturnThroughReference() != 3 || live != 0) return 11;
  {
    const std::string& s = std::string(40, 'x');
    if (s.size() != 40) return 12;
  }
  {
    const int& member = MakeHolder(5).tracked.id;
    if (live != 1 || member != 5) return 13;
  }
  if (live != 0) return 14;
  return 0;
}
