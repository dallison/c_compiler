// RUN: -std=c++26

#include <meta>

template <class T>
struct Box {
  T value;
};

#define REFLECT_TYPE(type) ^^type

constexpr auto macro_reflection = REFLECT_TYPE(Box<int>);
constexpr auto multiline_reflection =
    ^^Box<
        long>;

using MacroBox = typename [:macro_reflection:];
using MultilineBox = typename [:multiline_reflection:];

template <auto reflection>
struct Materialize {
  using type = typename [:reflection:];
};

using DependentBox = Materialize<^^Box<int>>::type;

static_assert(std::meta::is_same_type(macro_reflection, ^^Box<int>));
static_assert(std::meta::is_same_type(multiline_reflection, ^^Box<long>));
static_assert(std::meta::is_same_type(std::meta::dealias(^^DependentBox),
                                      macro_reflection));

#undef REFLECT_TYPE
