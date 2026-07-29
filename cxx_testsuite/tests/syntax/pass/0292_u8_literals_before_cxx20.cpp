// RUN: -std=c++17

template <class T, class U>
struct same_type {
  static constexpr bool value = false;
};

template <class T>
struct same_type<T, T> {
  static constexpr bool value = true;
};

static_assert(same_type<decltype(u8'x'), char>::value);
static_assert(same_type<decltype(u8"hi"), const char (&)[3]>::value);

const char* text = u8"UTF-8";
const char concatenated[] = u8"a" "b";
