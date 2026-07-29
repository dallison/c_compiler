// RUN: -std=c++20

#include <type_traits>

template <class T, class U>
struct same_type {
  static constexpr bool value = false;
};

template <class T>
struct same_type<T, T> {
  static constexpr bool value = true;
};

static_assert(sizeof(char8_t) == 1);
static_assert(alignof(char8_t) == 1);
static_assert(!same_type<char8_t, char>::value);
static_assert(!same_type<char8_t, unsigned char>::value);
static_assert(std::is_integral_v<char8_t>);
static_assert(std::is_unsigned_v<char8_t>);
static_assert(!std::is_signed_v<char8_t>);
static_assert(same_type<decltype(u8'x'), char8_t>::value);
static_assert(
    same_type<decltype(u8"hi"), const char8_t (&)[3]>::value);

struct char_result {};
struct unsigned_char_result {};
struct char8_result {};
struct int_result {};
struct unsigned_int_result {};

char_result select(char);
unsigned_char_result select(unsigned char);
char8_result select(char8_t);
int_result promote(int);
unsigned_int_result promote(unsigned int);

void check_overload() {
  char8_result result = select(u8'x');
  int_result promoted = promote(u8'x');
  (void)result;
  (void)promoted;
}

static_assert(same_type<decltype(u8'x' + 0), int>::value);

template <class T>
constexpr bool deduces_char8(T) {
  return same_type<T, char8_t>::value;
}

template <class T, unsigned long N>
constexpr bool deduces_char8_array(const T (&)[N]) {
  return same_type<T, char8_t>::value && N == 3;
}

static_assert(deduces_char8(u8'x'));
static_assert(deduces_char8_array(u8"hi"));

char8_t functional_cast = char8_t(65);
char8_t braced_cast = char8_t{66};
const char8_t initialized[] = u8"text";
const char8_t concatenated[] = u8"a" "b";
const char8_t raw[] = u8R"(raw UTF-8)";
