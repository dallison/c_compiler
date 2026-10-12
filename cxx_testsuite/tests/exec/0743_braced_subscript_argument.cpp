// RUN: -std=c++17
// EXPECT_EXIT: 0

struct Key {
  int first;
  int second;
};

struct Map {
  int operator[](Key key) const { return key.first + key.second; }
};

int main() {
  Map map;
  return map[{2, 3}] == 5 ? 0 : 1;
}
