// RUN: -std=c++26

#include <meta>

template <class T>
struct box {};

constexpr int reflected_object = 17;
constexpr int reflected_function(int value) { return value + 1; }

constexpr auto constant_info = std::meta::reflect_constant(42);
static_assert(std::meta::extract<int>(constant_info) == 42);

constexpr auto object_info = std::meta::reflect_object(reflected_object);
static_assert(std::meta::extract<const int&>(object_info) == 17);

constexpr auto function_info = std::meta::reflect_function(reflected_function);
constexpr auto function_pointer =
    std::meta::extract<int (*)(int)>(function_info);
static_assert(function_pointer(2) == 3);

static_assert(std::meta::can_substitute(^^box, {^^int}));
static_assert(std::meta::substitute(^^box, {^^int}) == ^^box<int>);

constexpr int values[] = {1, 2, 3};
constexpr auto static_values = std::define_static_array(values);
static_assert(static_values.size() == 3);
static_assert(static_values[2] == 3);

constexpr auto static_object = std::define_static_object(23);
static_assert(*static_object == 23);

int main() { return 0; }
