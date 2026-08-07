import std;

static_assert(std::is_same_v<
              std::ratio_add<std::ratio<1, 2>, std::ratio<1, 3>>,
              std::ratio<5, 6>>);
static_assert(std::tuple_size_v<std::tuple<int, long>> == 2);
static_assert(std::variant_size_v<std::variant<int, long>> == 2);
static_assert(std::is_same_v<std::expected<int, long>::error_type, long>);
static_assert(std::rotl<unsigned int>(0x80000001u, 1) == 3u);
static_assert(std::numbers::pi > 3.14159 && std::numbers::pi < 3.14160);
static_assert(std::is_same_v<std::syncbuf, std::basic_syncbuf<char>>);

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

  std::expected<int, long> outcome(6);
  std::expected<int, long> failure(std::unexpect, 7L);
  auto doubled = outcome.transform([](int item) { return item * 2; });
  if (!outcome || *outcome != 6 || failure || failure.error() != 7L ||
      !doubled || *doubled != 12) {
    return 6;
  }

  std::pmr::vector<int> polymorphic_values;
  polymorphic_values.push_back(9);
  if (polymorphic_values[0] != 9) {
    return 7;
  }
  std::pmr::string polymorphic_text(
      "module-visible polymorphic string storage");
  if (polymorphic_text[0] != 'm') {
    return 8;
  }

  unsigned int bits = std::bit_cast<unsigned int>(1.0f);
  if (bits != 0x3f800000u) {
    return 9;
  }

  return 0;
}
