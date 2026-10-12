// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <initializer_list>
#include <optional>
#include <type_traits>

struct Message {
  struct Impl {
    int first;
    long second;
  };

  Impl impl;
};

constexpr std::optional<unsigned long> Encode(
    std::initializer_list<unsigned long> offsets) {
  unsigned long bits = 0;
  for (unsigned long offset : offsets) {
    bits |= 1UL << offset;
  }
  return bits;
}

constexpr auto encoded =
    Encode({sizeof(Message::impl.first), sizeof(Message::impl.second)});

static_assert(std::is_same<decltype(Message::impl.second), long>::value);
static_assert(encoded.has_value());
static_assert(encoded.value() == ((1UL << sizeof(int)) | (1UL << sizeof(long))));

int main() {
  return 0;
}
