// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <expected>
#include <utility>

static_assert(__cpp_lib_expected == 202211L);

struct tracked {
  static int alive;
  int value;

  explicit tracked(int initial = 0) : value(initial) { ++alive; }
  tracked(const tracked& other) : value(other.value) { ++alive; }
  tracked(tracked&& other) noexcept : value(other.value) {
    other.value = -1;
    ++alive;
  }
  tracked& operator=(const tracked& other) {
    value = other.value;
    return *this;
  }
  tracked& operator=(tracked&& other) noexcept {
    value = other.value;
    other.value = -1;
    return *this;
  }
  ~tracked() { --alive; }

  friend bool operator==(const tracked& left, const tracked& right) {
    return left.value == right.value;
  }
};

int tracked::alive = 0;

struct move_only {
  int value;

  explicit move_only(int initial) : value(initial) {}
  move_only(const move_only&) = delete;
  move_only& operator=(const move_only&) = delete;
  move_only(move_only&& other) noexcept : value(other.value) {
    other.value = -1;
  }
  move_only& operator=(move_only&& other) noexcept {
    value = other.value;
    other.value = -1;
    return *this;
  }
};

constexpr int constexpr_sum() {
  std::expected<int, int> value(3);
  std::expected<int, int> error(std::unexpect, 4);
  return value.value_or(0) + error.error_or(0) + value.value() + error.error();
}

static_assert(constexpr_sum() == 14);

static int test_basics() {
  std::expected<int, int> value;
  if (!value || value.value() != 0 || value.value_or(3) != 0 ||
      value.error_or(7) != 7) {
    return 1;
  }

  std::expected<int, int> error(std::unexpect, 12);
  if (error || error.error() != 12 || error.value_or(5) != 5 ||
      error.error_or(9) != 12) {
    return 2;
  }

  std::unexpected deduced(14);
  std::expected<int, int> from_unexpected(deduced);
  if (from_unexpected || from_unexpected.error() != 14) {
    return 3;
  }

  bool caught = false;
  try {
    (void)error.value();
  } catch (const std::bad_expected_access<int>& exception) {
    caught = exception.error() == 12;
  }
  if (!caught) {
    return 4;
  }

  value = 8;
  error = value;
  if (!error || *error != 8) {
    return 5;
  }
  value = std::unexpected(17);
  if (value || value.error() != 17) {
    return 6;
  }
  value.emplace(19);
  if (!value || *value != 19) {
    return 7;
  }

  error = std::unexpected(8);
  value.swap(error);
  if (value || value.error() != 8 || !error || *error != 19) {
    return 8;
  }

  if (!(error == 19) || !(19 == error) ||
      !(value == std::unexpected(8)) ||
      !(std::unexpected(8) == value) || value == error) {
    return 9;
  }
  return 0;
}

static int test_lifetimes() {
  if (tracked::alive != 0) {
    return 20;
  }
  {
    std::expected<tracked, tracked> item(std::in_place, 1);
    std::expected<tracked, tracked> failure(std::unexpect, 2);
    if (tracked::alive != 2) {
      return 21;
    }
    item = failure;
    if (item || item.error().value != 2 || tracked::alive != 2) {
      return 22;
    }
    failure = std::expected<tracked, tracked>(std::in_place, 3);
    if (!failure || failure->value != 3 || tracked::alive != 2) {
      return 23;
    }
    item.swap(failure);
    if (!item || item->value != 3 || failure ||
        failure.error().value != 2 || tracked::alive != 2) {
      return 24;
    }
  }
  return tracked::alive == 0 ? 0 : 20 + tracked::alive;
}

static int test_move_only() {
  std::expected<move_only, move_only> value(std::in_place, 4);
  std::expected<move_only, move_only> error(std::unexpect, 5);
  error = std::move(value);
  if (!error || error->value != 4) {
    return 30;
  }
  error = std::unexpected(move_only(6));
  if (error || error.error().value != 6) {
    return 31;
  }
  return 0;
}

static int test_monadic() {
  std::expected<int, int> value(3);
  std::expected<int, int> error(std::unexpect, 4);

  auto chained = value.and_then(
      [](int& item) { return std::expected<long, int>(item * 2); });
  auto skipped = error.and_then(
      [](int&) { return std::expected<long, int>(99); });
  if (!chained || *chained != 6 || skipped || skipped.error() != 4) {
    return 40;
  }

  auto mapped = value.transform([](int item) { return item + 5; });
  auto mapped_error = error.transform([](int item) { return item + 5; });
  if (!mapped || *mapped != 8 || mapped_error ||
      mapped_error.error() != 4) {
    return 41;
  }

  int transformed_value = 0;
  auto void_result =
      value.transform([&](int item) { transformed_value = item; });
  if (!void_result || transformed_value != 3) {
    return 42;
  }

  auto recovered = error.or_else(
      [](int item) { return std::expected<int, long>(item + 10); });
  auto preserved = value.or_else(
      [](int) { return std::expected<int, long>(std::unexpect, 99L); });
  if (!recovered || *recovered != 14 || !preserved || *preserved != 3) {
    return 43;
  }

  auto changed_error = error.transform_error(
      [](int item) { return static_cast<long>(item + 20); });
  if (changed_error || changed_error.error() != 24L) {
    return 44;
  }

  const std::expected<int, int> const_value(7);
  auto const_chain = const_value.and_then(
      [](const int& item) { return std::expected<int, int>(item + 1); });
  auto moved_chain = std::expected<int, int>(9).and_then(
      [](int&& item) { return std::expected<int, int>(item + 1); });
  if (*const_chain != 8 || *moved_chain != 10) {
    return 45;
  }

  std::expected<void, int> done;
  std::expected<void, int> failed(std::unexpect, 11);
  int void_chain_calls = 0;
  auto void_chain = done.and_then([&] {
    ++void_chain_calls;
    return std::expected<int, int>(5);
  });
  auto void_skipped = failed.and_then([&] {
    ++void_chain_calls;
    return std::expected<int, int>(6);
  });
  if (!void_chain || *void_chain != 5 || void_skipped ||
      void_skipped.error() != 11 || void_chain_calls != 1) {
    return 46;
  }

  auto void_mapped = done.transform([] { return 15; });
  auto void_recovered = failed.or_else([](int item) {
    return item == 11 ? std::expected<void, long>()
                      : std::expected<void, long>(std::unexpect, 1L);
  });
  auto void_error = failed.transform_error(
      [](int item) { return static_cast<long>(item + 1); });
  if (!void_mapped || *void_mapped != 15 || !void_recovered ||
      void_error || void_error.error() != 12L) {
    return 47;
  }
  return 0;
}

int main() {
  int result = test_basics();
  if (result != 0) return result;
  result = test_lifetimes();
  if (result != 0) return result;
  result = test_move_only();
  if (result != 0) return result;
  return test_monadic();
}
