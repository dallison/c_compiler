import std;

int main() {
  std::array<int, 3> values{2, 3, 5};
  return values[0] + values[1] == values[2] ? 0 : 1;
}
