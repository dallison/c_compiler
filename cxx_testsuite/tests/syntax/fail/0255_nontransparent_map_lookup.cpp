// RUN: -std=c++20

#include <map>

struct Key {
  int value;
};

struct Probe {
  int value;
};

struct NonTransparentCompare {
  bool operator()(const Key&, const Key&) const;
  bool operator()(const Key&, const Probe&) const;
  bool operator()(const Probe&, const Key&) const;
};

void invalid_heterogeneous_lookup(
    std::map<Key, int, NonTransparentCompare>& map, const Probe& probe) {
  map.find(probe);
}
