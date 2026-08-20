// RUN: -std=c++26

#include <flat_map>
#include <map>
#include <unordered_map>
#include <version>

#ifdef __cpp_lib_map_lookup
#error "__cpp_lib_map_lookup must not be defined before C++29"
#endif

template <class Container>
concept has_lookup =
    requires(Container& container) { container.lookup(1); };

static_assert(!has_lookup<std::map<int, int>>);
static_assert(!has_lookup<std::unordered_map<int, int>>);
static_assert(!has_lookup<std::flat_map<int, int>>);
