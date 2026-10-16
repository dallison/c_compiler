// RUN: -std=c++20
// EXPECT_EXIT: 0

// [dcl.type.simple]: `signed int`, `short int`, `long int`, `unsigned`, ...
// name the same types as their shortest spellings, so they select the same
// explicit specializations and share instantiations.  Instantiating
// `HasStringify<signed int>` must not leave `declval<const int&>` producing a
// type that misses `IsHashable<int>` (abseil's Hash<int> under absl/log).
#include <type_traits>
#include <utility>

template <class T> struct S { static constexpr int v = 0; };
template <> struct S<int> { static constexpr int v = 1; };
template <> struct S<unsigned> { static constexpr int v = 2; };
template <> struct S<short> { static constexpr int v = 3; };
template <> struct S<unsigned short> { static constexpr int v = 4; };
template <> struct S<long> { static constexpr int v = 5; };
template <> struct S<unsigned long> { static constexpr int v = 6; };
template <> struct S<long long> { static constexpr int v = 7; };
template <> struct S<unsigned long long> { static constexpr int v = 8; };
template <> struct S<signed char> { static constexpr int v = 9; };
template <> struct S<char> { static constexpr int v = 10; };

static_assert(S<signed int>::v == 1);
static_assert(S<signed>::v == 1);
static_assert(S<int signed>::v == 1);
static_assert(S<unsigned int>::v == 2);
static_assert(S<short int>::v == 3);
static_assert(S<signed short int>::v == 3);
static_assert(S<unsigned short int>::v == 4);
static_assert(S<long int>::v == 5);
static_assert(S<signed long>::v == 5);
static_assert(S<unsigned long int>::v == 6);
static_assert(S<long long int>::v == 7);
static_assert(S<signed long long>::v == 7);
static_assert(S<unsigned long long int>::v == 8);
static_assert(S<signed char>::v == 9);
static_assert(S<char>::v == 10);

template <typename T, typename = void>
struct HasStringify : std::false_type {};
template <typename T>
struct HasStringify<
    T, std::enable_if_t<std::is_void<decltype(Stringify(
           std::declval<const T&>()))>::value>> : std::true_type {};

struct Log {
  Log& operator<<(signed int v) { return operator<< <signed int>(v); }
  template <typename T>
  Log& operator<<(const T& v);
  int count = 0;
};

template <typename T>
Log& Log::operator<<(const T&) {
  if constexpr (!HasStringify<T>::value) {
    ++count;
  }
  return *this;
}

template <class T> struct IsHashable : std::false_type {};
template <> struct IsHashable<int> : std::true_type {};

struct Probe {
  template <typename H, typename T>
  static auto Invoke(H state, const T&)
      -> std::enable_if_t<IsHashable<T>::value, H> {
    return state;
  }
};

static_assert(std::is_same_v<
              decltype(Probe::Invoke(1, std::declval<const int&>())), int>);

int main() {
  Log log;
  log << 3;
  return log.count == 1 ? 0 : 1;
}
