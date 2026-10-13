// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <algorithm>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

struct Entry {
  std::string name;
  int number;
};

struct EntryLess {
  bool operator()(const Entry& entry,
                  std::tuple<std::string_view, int> key) const {
    return std::make_tuple(std::string_view(entry.name), entry.number) < key;
  }

  bool operator()(std::tuple<std::string_view, int> key,
                  const Entry& entry) const {
    return key <
           std::make_tuple(std::string_view(entry.name), entry.number);
  }
};

int main() {
  Entry entries[] = {{"alpha", 1}, {"beta", 2}};
  auto key = std::make_pair(std::string("beta"), 2);
  auto found = std::lower_bound(entries, entries + 2, key, EntryLess{});
  return found == entries + 1 ? 0 : 1;
}
