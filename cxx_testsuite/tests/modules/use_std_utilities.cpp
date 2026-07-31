import std;

static_assert(std::is_same_v<
              std::ratio_add<std::ratio<1, 2>, std::ratio<1, 3>>,
              std::ratio<5, 6>>);
static_assert(std::tuple_size_v<std::tuple<int, long>> == 2);
static_assert(std::variant_size_v<std::variant<int, long>> == 2);

int main() {
  int values[] = {1, 2, 3, 4};
  std::span<int> view(values);
  view[1] = 7;
  if (view.size() != 4 || values[1] != 7) {
    return 1;
  }

  std::tuple<int, long> pair(3, 8L);
  if (std::get<0>(pair) != 3 || std::get<1>(pair) != 8L) {
    return 2;
  }

  std::variant<int, long> choice(11L);
  if (!std::holds_alternative<long>(choice) ||
      std::get<long>(choice) != 11L) {
    return 3;
  }

  std::source_location location = std::source_location::current();
  if (location.line() == 0 || location.file_name()[0] == '\0') {
    return 4;
  }

  std::runtime_error error("module error");
  if (error.what()[0] != 'm') {
    return 5;
  }

  return 0;
}
