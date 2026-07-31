import std;

int main() {
  std::vector<int> values{1, 2, 3};
  values.push_back(4);
  if (values.size() != 4 || values[0] != 1 || values[3] != 4) {
    return 1;
  }

  std::deque<int> queue;
  queue.push_back(5);
  queue.push_front(4);
  if (queue.size() != 2 || queue.front() != 4 || queue.back() != 5) {
    return 2;
  }

  std::string text("module");
  text.push_back('s');
  if (text.size() != 7 || text[0] != 'm' || text[6] != 's') {
    return 3;
  }

  std::optional<int> number(9);
  if (!number.has_value() || *number != 9) {
    return 4;
  }

  return 0;
}
