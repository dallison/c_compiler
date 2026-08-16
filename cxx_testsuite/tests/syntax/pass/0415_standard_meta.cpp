// RUN: -std=c++26

#include <initializer_list>
#include <meta>
#include <type_traits>
#include <version>

#if __cpp_lib_reflection != 202603L
#error "__cpp_lib_reflection has the wrong value"
#endif
#if __cpp_lib_define_static != 202506L
#error "__cpp_lib_define_static has the wrong value"
#endif
#if __cpp_lib_apply != 202506L
#error "__cpp_lib_apply has the wrong value"
#endif
#if __cpp_lib_integer_sequence != 202511L
#error "__cpp_lib_integer_sequence has the wrong value"
#endif

using std::meta::access_context;
using std::meta::data_member_options;
using std::meta::exception;
using std::meta::info;
using std::meta::member_offset;
using std::meta::operators;
using std::meta::reflection_range;

static_assert(std::is_same_v<info, decltype(^^int)>);
static_assert(std::is_enum_v<operators>);
static_assert(std::is_class_v<exception>);
static_assert(std::is_class_v<access_context>);
static_assert(std::is_class_v<member_offset>);
static_assert(std::is_class_v<data_member_options>);
static_assert(std::is_class_v<data_member_options::name_type>);

constexpr member_offset offset{4, 3};
static_assert(offset.total_bits() == 4 * CHAR_BIT + 3);

static_assert(std::meta::is_type(^^int));
static_assert(std::meta::is_type_alias(^^info));
static_assert(std::meta::dealias(^^info) == ^^decltype(^^::));
static_assert(reflection_range<std::initializer_list<info>>);

int main() {
  return 0;
}
