// RUN: -std=c++20 -fconstexpr-eval=audit

// __builtin_bit_cast reinterprets an object representation in a constant
// expression: scalars, arrays and classes (with bases) of the same size, from
// glvalue or prvalue operands, in templates and at namespace scope.
#include <bit>
#include <cstdint>

struct Halves {
  std::uint16_t low;
  std::uint16_t high;
};

struct Base {
  std::uint8_t tag;
};

struct Derived : Base {
  std::uint8_t a;
  std::uint16_t b;
};

struct Chars {
  char c[4];
};

static_assert(__builtin_bit_cast(std::uint32_t, 1.0f) == 0x3f800000u);
static_assert(__builtin_bit_cast(float, 0x3f800000u) == 1.0f);

constexpr Halves halves = std::bit_cast<Halves>(0x89abcdefu);
static_assert(halves.low == 0xcdef && halves.high == 0x89ab);

static_assert(std::bit_cast<std::uint32_t>(Halves{0x1111, 0x2222}) ==
              0x22221111u);

constexpr std::uint32_t from_derived() {
  Derived d{};
  d.tag = 1;
  d.a = 2;
  d.b = 0x0403;
  return std::bit_cast<std::uint32_t>(d);
}
static_assert(from_derived() == 0x04030201u);

constexpr bool to_derived() {
  Derived d = std::bit_cast<Derived>(0x0a0b0c0du);
  return d.tag == 0x0d && d.a == 0x0c && d.b == 0x0a0b;
}
static_assert(to_derived());

constexpr int round_trip_chars() {
  Chars chars{{'a', 'b', 'c', 'd'}};
  std::uint32_t bits = std::bit_cast<std::uint32_t>(chars);
  Chars back = std::bit_cast<Chars>(bits);
  return back.c[0] == 'a' && back.c[3] == 'd';
}
static_assert(round_trip_chars());

template <class To, class From>
constexpr To reinterpret(const From& from) {
  return __builtin_bit_cast(To, from);
}
static_assert(reinterpret<std::int32_t>(0xfffffffeu) == -2);
static_assert(reinterpret<double>(0x4000000000000000ull) == 2.0);

using Word = std::uint32_t[1];
constexpr std::uint32_t from_array() {
  Word word = {0xdeadbeefu};
  return std::bit_cast<std::uint32_t>(word);
}
static_assert(from_array() == 0xdeadbeefu);

struct Empty {
  constexpr int answer() const { return 42; }
};

struct WithEmptyBase : Empty {
  std::uint32_t value;
};

constexpr bool through_empty_base() {
  WithEmptyBase w{};
  w.value = 0x01020304u;
  WithEmptyBase back =
      std::bit_cast<WithEmptyBase>(std::bit_cast<std::uint32_t>(w));
  return back.value == 0x01020304u && back.answer() == 42;
}
static_assert(through_empty_base());
