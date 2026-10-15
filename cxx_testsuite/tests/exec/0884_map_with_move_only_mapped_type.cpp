// `std::map::operator[]`, `try_emplace` and `insert_or_assign` move the
// mapped value into the node; a move-only mapped type needs no copy
// constructor.
#include <map>
#include <memory>
#include <string>
#include <utility>

struct MoveOnly {
  std::unique_ptr<int> p;
  MoveOnly() = default;
  explicit MoveOnly(int v) : p(new int(v)) {}
  MoveOnly(MoveOnly&&) = default;
  MoveOnly& operator=(MoveOnly&&) = default;
};

int main() {
  std::map<int, MoveOnly> m;
  m[1] = MoveOnly(4);
  if (*m[1].p != 4 || m.size() != 1) return 1;
  m[2].p.reset(new int(5));
  if (*m[2].p != 5 || m.size() != 2) return 2;
  if (!m.try_emplace(3, 6).second || *m[3].p != 6) return 3;
  if (m.try_emplace(3, 7).second || *m[3].p != 6) return 4;
  m.insert_or_assign(4, MoveOnly(8));
  m.insert_or_assign(4, MoveOnly(9));
  if (*m[4].p != 9 || m.size() != 4) return 5;

  std::map<std::string, std::unique_ptr<int>> n;
  n["a"] = std::make_unique<int>(5);
  if (*n["a"] != 5 || n["b"] != nullptr || n.size() != 2) return 6;

  std::map<int, int> counts;
  counts[7]++;
  counts[7]++;
  if (counts[7] != 2 || counts[8] != 0) return 7;
  return 0;
}
