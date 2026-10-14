// RUN: -std=c++17
// EXPECT_EXIT: 0

// An empty braced list value-initializes a class that has a default
// constructor; initializer-list constructors are not considered
// ([dcl.init.list]/3.4).  Choosing vector's initializer_list constructor for
// `{}` instantiated its element copy, which is ill-formed for move-only
// elements such as unique_ptr.

#include <initializer_list>
#include <memory>
#include <vector>

int chosen;

struct L {
  L() { chosen = 1; }
  L(std::initializer_list<int>) { chosen = 2; }
};

struct OnlyList {
  OnlyList(std::initializer_list<int>) { chosen = 3; }
};

struct HoldsL {
  L l;
};

L ReturnEmpty() { return {}; }
void TakeL(L) {}

struct Entries {
  std::vector<std::unique_ptr<int>> owned;
};

Entries Make() {
  Entries e;
  e.owned.push_back(std::unique_ptr<int>(new int(1)));
  return e;
}

int main(int argc, char**) {
  chosen = 0;
  L direct{};
  if (chosen != 1) return 1;
  chosen = 0;
  L copy_list = {};
  if (chosen != 1) return 2;
  chosen = 0;
  HoldsL aggregate{};
  if (chosen != 1) return 3;
  chosen = 0;
  ReturnEmpty();
  if (chosen != 1) return 4;
  chosen = 0;
  TakeL({});
  if (chosen != 1) return 5;
  chosen = 0;
  L one{1};
  if (chosen != 2) return 6;
  chosen = 0;
  OnlyList only{};
  if (chosen != 3) return 7;

  std::vector<std::unique_ptr<int>> empty{};
  Entries value_initialized{};
  const Entries picked = argc > 0 ? Make() : Entries();
  if (!empty.empty() || !value_initialized.owned.empty()) return 8;
  if (picked.owned.size() != 1 || *picked.owned[0] != 1) return 9;
  return 0;
}
