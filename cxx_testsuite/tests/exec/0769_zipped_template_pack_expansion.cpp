#include <type_traits>

template <class...>
struct Tuple {};

template <bool, class, class...>
struct ConstructibleZip : std::false_type {};

template <class... Ts, class... Us>
struct ConstructibleZip<true, Tuple<Ts...>, Us...>
    : std::conjunction<std::is_constructible<Ts, Us&&>...> {};

struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&&) = default;
};

static_assert(
    ConstructibleZip<true, Tuple<MoveOnly, int>, MoveOnly, int>::value);

int main() {}
