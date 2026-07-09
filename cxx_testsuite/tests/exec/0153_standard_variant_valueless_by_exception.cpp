// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <variant>
#include <utility>

bool g_variant_throw_on_copy = false;
bool g_variant_throw_on_move = false;
int g_variant_thrower_live = 0;

struct Thrower {
  int value;

  explicit Thrower(int v) : value(v) {
    ++g_variant_thrower_live;
  }

  Thrower(const Thrower& other) : value(other.value) {
    if (g_variant_throw_on_copy) {
      throw 153;
    }
    ++g_variant_thrower_live;
  }

  Thrower(Thrower&& other) : value(other.value) {
    if (g_variant_throw_on_move) {
      throw 154;
    }
    ++g_variant_thrower_live;
    other.value = -1;
  }

  Thrower& operator=(const Thrower& other) {
    if (g_variant_throw_on_copy) {
      throw 155;
    }
    value = other.value;
    return *this;
  }

  Thrower& operator=(Thrower&& other) {
    if (g_variant_throw_on_move) {
      throw 156;
    }
    value = other.value;
    other.value = -1;
    return *this;
  }

  ~Thrower() {
    --g_variant_thrower_live;
  }
};

int main(void) {
  {
    std::variant<int, Thrower> src(std::in_place_index<1>, 7);
    std::variant<int, Thrower> dst(3);

    g_variant_throw_on_copy = true;
    bool caught = false;
    try {
      dst = src;
    } catch (int code) {
      caught = code == 153;
    }
    g_variant_throw_on_copy = false;

    if (!caught || !dst.valueless_by_exception() ||
        dst.index() != std::variant_npos) {
      return 1;
    }

    dst.emplace<0>(11);
    if (dst.index() != 0 || std::get<0>(dst) != 11) {
      return 2;
    }
  }

  if (g_variant_thrower_live != 0) {
    return 3;
  }

  {
    std::variant<int, Thrower> src(std::in_place_index<1>, 9);
    std::variant<int, Thrower> dst(4);

    g_variant_throw_on_move = true;
    bool caught = false;
    try {
      dst = std::move(src);
    } catch (int code) {
      caught = code == 154;
    }
    g_variant_throw_on_move = false;

    if (!caught || !dst.valueless_by_exception() ||
        dst.index() != std::variant_npos) {
      return 4;
    }

    dst.emplace<0>(12);
    if (dst.index() != 0 || std::get<0>(dst) != 12) {
      return 5;
    }
  }

  if (g_variant_thrower_live != 0) {
    return 6;
  }

  {
    std::variant<int, Thrower> value(std::in_place_index<1>, 13);
    g_variant_throw_on_copy = true;
    bool caught = false;
    try {
      value = Thrower(14);
    } catch (int code) {
      caught = code == 156 || code == 155 || code == 153 || code == 154;
    }
    g_variant_throw_on_copy = false;
    if (caught || value.valueless_by_exception() ||
        std::get<1>(value).value != 14) {
      return 7;
    }
  }

  return g_variant_thrower_live == 0 ? 0 : 8;
}
