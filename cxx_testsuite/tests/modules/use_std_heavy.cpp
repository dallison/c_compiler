import std;

std::generator<int> module_numbers() {
  co_yield 2;
  co_yield 3;
  co_yield 5;
}

int module_square(int value) {
  return value * value;
}

int main() {
  std::list<int> linked{1, 2};
  linked.push_back(3);
  if (linked.size() != 3 || linked.front() != 1 || linked.back() != 3) {
    return 1;
  }

  std::map<int, int> ordered;
  ordered[2] = 4;
  std::unordered_map<int, int> hashed;
  hashed[3] = 9;
  std::set<int> unique{4, 4, 5};
  std::unordered_set<int> hashed_unique{6, 6, 7};
  if (ordered[2] != 4 || hashed[3] != 9 || unique.size() != 2 ||
      hashed_unique.size() != 2) {
    return 2;
  }

  std::queue<int> fifo;
  fifo.push(8);
  fifo.push(9);
  std::stack<int> lifo;
  lifo.push(8);
  lifo.push(9);
  if (fifo.front() != 8 || lifo.top() != 9) {
    return 3;
  }

  if (std::invoke(&module_square, 6) != 36) {
    return 4;
  }

  int values[] = {4, 1, 3, 2};
  std::ranges::sort(values);
  if (values[0] != 1 || values[3] != 4 ||
      !std::ranges::contains(values, 3)) {
    return 5;
  }

  auto add = [](int left, int right) { return left + right; };
  if (std::ranges::fold_left(values, 0, add) != 10) {
    return 6;
  }
  auto copied = std::ranges::to<std::vector<int>>(values);
  if (copied.size() != 4 || copied[2] != 3) {
    return 7;
  }

  auto sequence = module_numbers();
  auto current = sequence.begin();
  int generated = *current;
  ++current;
  generated += *current;
  ++current;
  generated += *current;
  if (generated != 10) {
    return 8;
  }

  if (std::format("{} + {} = {}", 2, 3, 5) != "2 + 3 = 5") {
    return 9;
  }
  std::println("import std heavy surface: {}", generated);
  std::println();
  return 0;
}
