#!/bin/bash
set -euo pipefail

DAVECC="$1"

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/cxx-lexical-mode.XXXXXX")"

expect_compile() {
  local name="$1"
  local source="$2"
  shift 2
  local src="$WORK/$name.c"
  printf '%s\n' "$source" > "$src"
  "$ROOT/$DAVECC" -target pcode -S "$@" "$src" -o "$WORK/$name.s" \
      >"$WORK/$name.out" 2>&1
}

expect_fail() {
  local name="$1"
  local source="$2"
  shift 2
  local src="$WORK/$name.c"
  printf '%s\n' "$source" > "$src"
  set +e
  "$ROOT/$DAVECC" -target pcode -S "$@" "$src" -o "$WORK/$name.s" \
      >"$WORK/$name.out" 2>&1
  local status=$?
  set -e
  local output
  output="$(<"$WORK/$name.out")"
  if [[ "$status" -eq 0 && "$output" != *"error:"* ]]; then
    echo "$name: expected compilation failure" >&2
    exit 1
  fi
}

expect_cpp20_extension() {
  local extension="$1"
  local src="$WORK/default_cpp20.$extension"
  printf '%s\n' \
    '#if __cplusplus != 202002L' \
    '#error expected C++20 mode' \
    '#endif' \
    'namespace inferred { int value = 0; }' \
    'int main(void) { return inferred::value; }' > "$src"
  "$ROOT/$DAVECC" -target pcode -S "$src" \
      -o "$WORK/default_cpp20_$extension.s" \
      >"$WORK/default_cpp20_$extension.out" 2>&1
}

expect_cpp20_extension cc
expect_cpp20_extension cpp

iostream_src="$WORK/default_iostream.cc"
printf '%s\n' \
  '#include <iostream>' \
  'int main(void) { std::cout << "hello world\n"; }' > "$iostream_src"
"$ROOT/$DAVECC" -target aarch64 -S -isystem "$ROOT/libc/include" \
    "$iostream_src" -o "$WORK/default_iostream.s" \
    >"$WORK/default_iostream.out" 2>&1
if [[ -s "$WORK/default_iostream.out" ]]; then
  echo "default_iostream: unexpected diagnostics" >&2
  sed 's/^/  /' "$WORK/default_iostream.out" >&2
  exit 1
fi

expect_compile c_mode_class \
  'int class; int main(void) { class = 3; return class; }'
expect_fail cxx11_class \
  'int class; int main(void) { return 0; }' \
  -std=c++11

expect_compile cxx17_concept \
  'int concept; int main(void) { concept = 4; return concept; }' \
  -std=c++17
expect_fail cxx20_concept \
  'int concept; int main(void) { return 0; }' \
  -std=c++20

expect_compile cxx17_char8_t \
  'int char8_t; int main(void) { char8_t = 5; return char8_t; }' \
  -std=c++17
expect_fail cxx20_char8_t \
  'int char8_t; int main(void) { return 0; }' \
  -std=c++20

expect_compile c_mode_alt_operator_word \
  'int and; int main(void) { and = 1; return and; }'
expect_fail cxx11_alt_operator_word \
  'int and = 1; int main(void) { return and; }' \
  -std=c++11

expect_compile cxx11_alt_operator_expr \
  'int main(void) { return (1 and 1) && (1 not_eq 0); }' \
  -std=c++11

expect_fail cxx11_digit_separator \
  "int main(void) { return 1'000; }" \
  -std=c++11
expect_compile cxx14_digit_separator \
  "int main(void) { return 1'000 == 1000 ? 0 : 1; }" \
  -std=c++14
expect_fail cxx11_binary_literal \
  'int main(void) { return 0b1010; }' \
  -std=c++11
expect_compile cxx14_binary_literal \
  "int main(void) { return 0b1010'0101 == 165 ? 0 : 1; }" \
  -std=c++14
expect_compile cxx14_macro_pp_number \
  "constexpr unsigned long long operator\"\"_suffix(unsigned long long value) { return value; }
#define N 0b1010'0101_suffix
int main(void) { return N == 165 ? 0 : 1; }" \
  -std=c++14

expect_fail c_mode_user_defined_literal \
  'int main(void) { return 123_km; }'
expect_compile cxx11_user_defined_literal \
  'constexpr unsigned long long operator""_km(unsigned long long value) { return value; }
int main(void) { return 123_km == 123 ? 0 : 1; }' \
  -std=c++11

expect_compile cxx11_prefixed_literals \
  'int main(void) { u8"text"; u"text"; U"text"; u8'"'"'x'"'"'; u'"'"'x'"'"'; U'"'"'x'"'"'; return 0; }' \
  -std=c++11
expect_compile cxx11_raw_literals \
  'int main(void) { R"delim(raw \ text)delim"; u8R"(raw)"; LR"(raw)"; return 0; }' \
  -std=c++11

multiline_raw="$WORK/cxx11_multiline_raw.c"
printf '%s\n' \
  'int main(void) { R"raw(first line' \
  'second line)raw"; return 0; }' > "$multiline_raw"
"$ROOT/$DAVECC" -target pcode -S -std=c++11 "$multiline_raw" \
    -o "$WORK/cxx11_multiline_raw.s" >"$WORK/cxx11_multiline_raw.out" 2>&1

macro_raw="$WORK/cxx11_macro_raw.c"
printf '%s\n' \
  '#define RAW R"raw(a"b)raw"' \
  'int main(void) { RAW; return 0; }' > "$macro_raw"
"$ROOT/$DAVECC" -target pcode -S -std=c++11 "$macro_raw" \
    -o "$WORK/cxx11_macro_raw.s" >"$WORK/cxx11_macro_raw.out" 2>&1

printf '%s\n' 'int pp_header_value(void) { return 0; }' > "$WORK/pp_header.h"
include_header="$WORK/cxx11_include_header.c"
printf '%s\n' \
  '#include <pp_header.h>' \
  'int main(void) { return pp_header_value(); }' > "$include_header"
"$ROOT/$DAVECC" -target pcode -S -std=c++11 -isystem "$WORK" "$include_header" \
    -o "$WORK/cxx11_include_header.s" >"$WORK/cxx11_include_header.out" 2>&1

expect_compile cxx17_module_identifiers \
  'int module; int import; int main(void) { module = 1; import = 2; return module + import; }' \
  -std=c++17
expect_compile cxx20_module_keywords \
  'int module; int import; int main(void) { return 0; }' \
  -std=c++20

expect_compile cxx11_constexpr \
  'constexpr int value = 1; int main(void) { return value; }' \
  -std=c++11

expect_compile cxx26_mode \
  '#if __cplusplus != 202603L
#error expected C++26 mode
#endif
#if __cpp_pp_embed != 202502L
#error expected #embed feature macro
#endif
#if __cpp_pack_indexing != 202311L
#error expected pack indexing feature macro
#endif
#if __cpp_structured_bindings != 202411L
#error expected C++26 structured bindings feature macro
#endif
#if __cpp_variadic_friend != 202403L
#error expected variadic friend feature macro
#endif
#if __cpp_deleted_function != 202403L
#error expected deleted function feature macro
#endif
#if __cpp_static_assert != 202306L
#error expected user-generated static_assert message feature macro
#endif
#if __cpp_placeholder_variables != 202306L
#error expected placeholder variables feature macro
#endif
#if __cpp_expansion_statements != 202506L
#error expected expansion statements feature macro
#endif
int main(void) { return 0; }' \
  -std=c++26
expect_compile cxx2c_mode_alias \
  '#if __cplusplus != 202603L
#error expected C++26 mode
#endif
int main(void) { return 0; }' \
  -std=c++2c

expect_compile cxx26_variadic_friend \
  'struct Audit;
template<class... Friends>
struct vault {
  friend Friends..., Audit;
};
template<class Owner, class Tag> struct receiver {};
template<class... Tags>
struct dispatcher {
  friend receiver<dispatcher, Tags>...;
};
struct nested_friend { struct type {}; };
template<class... Owners>
struct nested_owner {
  friend typename Owners::type...;
};
vault<Audit> v;
dispatcher<int, char> d;
nested_owner<nested_friend> n;
int main(void) { return 0; }' \
  -std=c++26
expect_compile cxx26_friend_type_list \
  'struct Second;
struct owner {
  friend class First, Second;
};
int main(void) { return 0; }' \
  -std=c++26
expect_fail cxx23_no_variadic_friend \
  'template<class... Friends>
struct owner { friend Friends...; };
int main(void) { return 0; }' \
  -std=c++23
expect_fail cxx26_variadic_friend_requires_pack \
  'template<class Friend>
struct owner { friend Friend...; };
int main(void) { return 0; }' \
  -std=c++26
expect_fail cxx26_friend_type_own_template_parameter \
  'struct owner {
  template<class... Friends>
  friend Friends...;
};
int main(void) { return 0; }' \
  -std=c++26
expect_fail cxx26_friend_function_still_requires_typename \
  'template<class T>
struct owner { friend T::type function(); };
int main(void) { return 0; }' \
  -std=c++26
expect_fail cxx23_no_friend_type_only_context \
  'template<class T>
struct owner { friend T::type; };
int main(void) { return 0; }' \
  -std=c++23
expect_compile cxx23_no_variadic_friend_macro \
  '#ifdef __cpp_variadic_friend
#error variadic friend macro must not be defined before C++26
#endif
int main(void) { return 0; }' \
  -std=c++23
expect_compile cxx26_deleted_function_reason \
  'void legacy(int) = delete("use the long overload");
template<class T>
void unsupported(T) = delete("this type is unsupported");
struct move_only {
  move_only() = default;
  move_only(const move_only&) = delete("use move construction");
  void reset(int) = delete("call reset() with no arguments");
};
int main(void) { return 0; }' \
  -std=c++26
expect_fail cxx23_no_deleted_function_reason \
  'void legacy() = delete("requires C++26");
int main(void) { return 0; }' \
  -std=c++23
expect_fail cxx26_deleted_function_reason_requires_string \
  'void legacy() = delete(42);
int main(void) { return 0; }' \
  -std=c++26
expect_compile cxx23_no_deleted_function_macro \
  '#ifdef __cpp_deleted_function
#error deleted function macro must not be defined before C++26
#endif
int main(void) { return 0; }' \
  -std=c++23
expect_compile cxx26_static_assert_generated_message \
  'struct unused_message {};
static_assert(true, unused_message{});
int main(void) { return 0; }' \
  -std=c++26
expect_fail cxx23_no_static_assert_generated_message \
  'struct message {};
static_assert(false, message{});
int main(void) { return 0; }' \
  -std=c++23
expect_fail cxx26_static_assert_message_size_constant \
  'struct message {
  constexpr const char* data() const { return "bad"; }
  const char* size() const { return "three"; }
};
static_assert(false, message{});
int main(void) { return 0; }' \
  -std=c++26
expect_fail cxx26_static_assert_message_size_nonconstant \
  'struct message {
  constexpr const char* data() const { return "bad"; }
  unsigned size() const { return 3; }
};
static_assert(false, message{});
int main(void) { return 0; }' \
  -std=c++26
expect_fail cxx26_static_assert_message_data_pointer \
  'struct message {
  constexpr int data() const { return 0; }
  constexpr unsigned size() const { return 0; }
};
static_assert(false, message{});
int main(void) { return 0; }' \
  -std=c++26
expect_compile cxx23_static_assert_macro \
  '#if __cpp_static_assert != 201411L
#error expected C++17 static_assert feature macro value
#endif
int main(void) { return 0; }' \
  -std=c++23

expect_compile cxx26_placeholder_variables \
  'struct holder {
  int _;
  long _;
};
class intentionally_ignored {
  int _;
  long _;
};
int main() {
  int _;
  double _;
  auto [_, _] = holder{1, 2};
  static auto [_, _] = holder{3, 4};
  auto closure = [_ = 1, _ = 2] { return 0; };
  return closure();
}' \
  -std=c++26 -Wall -Werror
expect_compile cxx23_no_placeholder_variables_macro \
  '#ifdef __cpp_placeholder_variables
#error placeholder variables macro must not be defined before C++26
#endif
int main(void) { return 0; }' \
  -std=c++23
expect_compile cxx23_no_expansion_statements_macro \
  '#ifdef __cpp_expansion_statements
#error expansion statements macro must not be defined before C++26
#endif
int main(void) { return 0; }' \
  -std=c++23
expect_fail cxx23_no_expansion_statement \
  'int main() {
  template for (auto x : {1, 2, 3}) {
    (void)x;
  }
  return 0;
}' \
  -std=c++23
expect_fail cxx23_no_placeholder_variable_redeclaration \
  'int main() {
  int _;
  double _;
  return 0;
}' \
  -std=c++23
expect_fail cxx26_placeholder_parameter_not_name_independent \
  'void consume(int _, long _) {}
int main(void) { return 0; }' \
  -std=c++26
expect_fail cxx26_namespace_placeholder_not_name_independent \
  'int _ = 1;
long _ = 2;
int main(void) { return 0; }' \
  -std=c++26

expect_compile cxx26_pack_indexing \
  'template<class T> struct type_tag;
template<> struct type_tag<int> { static constexpr int value = 11; };
template<> struct type_tag<char> { static constexpr int value = 22; };
template<> struct type_tag<const int> { static constexpr int value = 33; };
template<class T> struct selected_type_tag;
template<> struct selected_type_tag<char> {};
struct pack_base { int member; };
template<class... Bases> struct derived : Bases...[0] {};
struct member_holder { using value_type = int; };
template<class... Ts>
using first_member_t = typename Ts...[0]::value_type;

template<class... Ts>
constexpr int first_type_value() {
  return type_tag<Ts...[0]>::value;
}
template<class... Ts>
using const_first_t = Ts...[0] const;

template<class... Ts>
constexpr int last_type_value() {
  return type_tag<Ts...[sizeof...(Ts) - 1]>::value;
}

template<int I, class... Ts>
constexpr int select_argument(Ts... values) {
  return values...[I];
}
template<int I, class... Ts>
int check_selected_type(Ts... values) {
  selected_type_tag<decltype(values...[I])> selected;
  return sizeof(selected) == 0;
}

template<class... Ts>
constexpr int select_last_argument(Ts... values) {
  return values...[sizeof...(Ts) - 1];
}

template<int... Values>
constexpr int select_value() {
  return Values...[1];
}

template<int... Is> struct index_pack {};
template<class... Ts> struct type_pack {};
template<class T> struct order_tag;
template<> struct order_tag<type_pack<char, int>> {
  static constexpr int value = 1;
};
constexpr int combine(int first, int second) { return first * 10 + second; }
template<class... Ts>
constexpr int non_deduced_first(Ts...[0], type_pack<Ts...>*) {
  return sizeof...(Ts);
}
template<class Indexes, class Types> struct reordered;
template<int... Is, class... Ts>
struct reordered<index_pack<Is...>, type_pack<Ts...>> {
  using types = type_pack<Ts...[Is]...>;
  static constexpr int values(Ts... values) {
    return combine(values...[Is]...);
  }
};

template<class T> struct category_tag;
template<> struct category_tag<int> { static constexpr int value = 1; };
template<> struct category_tag<int&> { static constexpr int value = 2; };
template<class... Ts>
int check_decltype_category(Ts... values) {
  category_tag<decltype(values...[0])> unparenthesized;
  category_tag<decltype((values...[0]))> parenthesized;
  return sizeof(unparenthesized) == sizeof(parenthesized) ? 0 : 1;
}

static_assert(first_type_value<int, char>() == 11);
static_assert(type_tag<const_first_t<int>>::value == 33);
static_assert(type_tag<first_member_t<member_holder>>::value == 11);
static_assert(last_type_value<int, char>() == 22);
static_assert(select_argument<1>(3, 7, 9) == 7);
static_assert(select_last_argument(3, 7, 9) == 9);
static_assert(select_value<4, 8, 12>() == 8);
using reordered_type =
    reordered<index_pack<1, 0>, type_pack<int, char>>::types;
static_assert(order_tag<reordered_type>::value == 1);
static_assert(
    reordered<index_pack<1, 0>, type_pack<int, int>>::values(3, 7) == 73);
static_assert(non_deduced_first(1, (type_pack<int, char>*)0) == 2);
int main(void) {
  derived<pack_base> object;
  object.member = 3;
  return check_decltype_category(5) +
         check_selected_type<1>(1, (char)2) + object.member - 3;
}' \
  -std=c++26

expect_compile cxx23_no_pack_indexing_macro \
  '#ifdef __cpp_pack_indexing
#error pack indexing macro must not be defined before C++26
#endif
int main(void) { return 0; }' \
  -std=c++23
expect_compile cxx17_structured_bindings_macro \
  '#if __cpp_structured_bindings != 201606L
#error expected C++17 structured bindings feature macro
#endif
int main(void) { return 0; }' \
  -std=c++17
expect_compile cxx23_structured_bindings_macro \
  '#if __cpp_structured_bindings != 201606L
#error expected pre-C++26 structured bindings feature macro
#endif
int main(void) { return 0; }' \
  -std=c++23
expect_compile cxx14_no_structured_bindings_macro \
  '#ifdef __cpp_structured_bindings
#error structured bindings macro must not be defined before C++17
#endif
int main(void) { return 0; }' \
  -std=c++14
expect_compile cxx26_structured_binding_attributes \
  'struct pair { int first, second; };
int main() {
  auto [first [[maybe_unused]], second] = pair{1, 2};
  return second - 2;
}' \
  -std=c++26
expect_fail cxx23_no_structured_binding_attributes \
  'struct pair { int first, second; };
int main() {
  auto [first [[maybe_unused]], second] = pair{1, 2};
  return second - 2;
}' \
  -std=c++23
expect_fail cxx23_no_structured_binding_pack \
  'struct triple { int a, b, c; };
template<class T> int f(T value) {
  auto [...items] = value;
  return (items + ...);
}
int main() { return f(triple{1, 2, 3}); }' \
  -std=c++23
expect_fail cxx26_structured_binding_pack_requires_template \
  'struct triple { int a, b, c; };
int main() {
  auto [...items] = triple{1, 2, 3};
  return (items + ...);
}' \
  -std=c++26
expect_fail cxx26_structured_binding_multiple_packs \
  'struct triple { int a, b, c; };
template<class T> int f(T value) {
  auto [...left, ...right] = value;
  return 0;
}
int main() { return f(triple{1, 2, 3}); }' \
  -std=c++26
expect_fail cxx26_structured_binding_too_many_fixed \
  'struct pair { int a, b; };
template<class T> int f(T value) {
  auto [a, b, c, ...rest] = value;
  return 0;
}
int main() { return f(pair{1, 2}); }' \
  -std=c++26
expect_fail cxx26_structured_binding_pack_index_out_of_bounds \
  'struct pair { int a, b; };
template<class T> int f(T value) {
  auto [...items] = value;
  return items...[2];
}
int main() { return f(pair{1, 2}); }' \
  -std=c++26
expect_fail cxx26_structured_binding_undecomposable \
  'template<class T> int f(T value) {
  auto [...items] = value;
  return 0;
}
int main() { return f(1); }' \
  -std=c++26
expect_fail cxx26_structured_binding_inaccessible_member \
  'class hidden_pair {
    int a;
    int b;
  public:
    hidden_pair(int x, int y) : a(x), b(y) {}
  };
template<class T> int f(T value) {
  auto [...items] = value;
  return 0;
}
int main() { return f(hidden_pair(1, 2)); }' \
  -std=c++26
expect_fail cxx26_type_pack_index_out_of_bounds \
  'template<class... Ts> using third_t = Ts...[2];
third_t<int, char> value;
int main(void) { return 0; }' \
  -std=c++26
expect_fail cxx26_expression_pack_index_out_of_bounds \
  'template<int I, class... Ts>
constexpr int select(Ts... values) { return values...[I]; }
static_assert(select<2>(1, 2) == 0);
int main(void) { return 0; }' \
  -std=c++26
expect_fail cxx26_pack_index_not_constant \
  'template<class... Ts>
int select(int i, Ts... values) { return values...[i]; }
int main(void) { return select(0, 1); }' \
  -std=c++26
expect_fail cxx26_pack_index_empty_pack \
  'template<class... Ts>
constexpr int select(Ts... values) { return values...[0]; }
static_assert(select() == 0);
int main(void) { return 0; }' \
  -std=c++26
expect_fail cxx26_pack_index_requires_pack \
  'template<class T> using indexed = T...[0];
indexed<int> value;
int main(void) { return 0; }' \
  -std=c++26
expect_fail cxx26_pack_index_requires_unparenthesized_name \
  'template<class... Ts>
int select(Ts... values) { return (values)...[0]; }
int main(void) { return select(1); }' \
  -std=c++26
expect_fail cxx26_pack_index_is_non_deduced \
  'template<class... Ts> void only(Ts...[0]);
int main(void) { only(1); }' \
  -std=c++26

printf '\x00\x01\x7f\x80\xff' > "$WORK/embed.bin"
: > "$WORK/empty.bin"
mkdir "$WORK/unprocessable.bin"
expect_compile cxx26_embed \
  '#ifndef __has_embed
#error "__has_embed must be defined in C++26"
#endif
#if __has_embed("embed.bin") != __STDC_EMBED_FOUND__
#error "embed.bin must be found and non-empty"
#endif
#if __has_embed("empty.bin") != __STDC_EMBED_EMPTY__
#error "empty.bin must be found and empty"
#endif
#if __has_embed("missing.bin") != __STDC_EMBED_NOT_FOUND__
#error "missing resource must not be found"
#endif
#if __has_embed(<embed.bin>) != __STDC_EMBED_FOUND__
#error "angle resource search must use system paths"
#endif
#if __has_embed("embed.bin" davecc::unknown(1)) != __STDC_EMBED_NOT_FOUND__
#error "unsupported vendor parameters must report not found"
#endif
#if __has_embed("embed.bin" limit(0)) != __STDC_EMBED_EMPTY__
#error "limit(0) must make the resource empty"
#endif
static const unsigned char all[] = {
#embed "embed.bin"
};
static_assert(sizeof(all) == 5);
constexpr int first =
#embed "embed.bin" limit(1)
;
static_assert(first == 0);
constexpr int wrapped =
#embed "embed.bin" limit(1) prefix(1 +) suffix(+ 2)
;
static_assert(wrapped == 3);
static const unsigned char slice[] = {
#embed "embed.bin" limit(2) prefix(9,) suffix(,10)
};
static_assert(sizeof(slice) == 4);
static const unsigned char uintmax_limit[] = {
#embed "embed.bin" limit(18446744073709551615ULL)
};
static_assert(sizeof(uintmax_limit) == 5);
constexpr int empty_fallback =
#embed "empty.bin" if_empty(42)
;
static_assert(empty_fallback == 42);
#define EMBED_RESOURCE "embed.bin"
#define EMBED_LIMIT (1 + 1)
static const unsigned char macro_resource[] = {
#embed EMBED_RESOURCE limit(EMBED_LIMIT)
};
static_assert(sizeof(macro_resource) == 2);
int main(void) { return 0; }' \
  -std=c++26 -I"$WORK" -isystem "$WORK"

expect_fail cxx23_embed \
  'constexpr unsigned char data[] = {
#embed "embed.bin"
};' \
  -std=c++23 -I"$WORK"
expect_fail cxx26_embed_missing \
  'static const unsigned char data[] = {
#embed "missing.bin"
};' \
  -std=c++26 -I"$WORK"
expect_fail cxx26_embed_duplicate_parameter \
  'static const unsigned char data[] = {
#embed "embed.bin" limit(1) limit(2)
};' \
  -std=c++26 -I"$WORK"
expect_fail cxx26_embed_negative_limit \
  'static const unsigned char data[] = {
#embed "embed.bin" limit(-1)
};' \
  -std=c++26 -I"$WORK"
expect_fail cxx26_embed_unbalanced_parameter \
  '#if __has_embed("embed.bin" prefix({))
#endif
int main(void) { return 0; }' \
  -std=c++26 -I"$WORK"
expect_fail cxx26_has_embed_unprocessable \
  '#if __has_embed("unprocessable.bin")
#endif
int main(void) { return 0; }' \
  -std=c++26 -I"$WORK"
expect_fail cxx26_has_embed_unprocessable_unsupported_parameter \
  '#if __has_embed("unprocessable.bin" davecc::unknown)
#endif
int main(void) { return 0; }' \
  -std=c++26 -I"$WORK"

if "$ROOT/$DAVECC" -target pcode -std=c++29 -S "$WORK/no_such.c" \
    -o "$WORK/no_such.s" >"$WORK/bad_std.out" 2>&1; then
  echo "bad_std: expected invalid -std failure" >&2
  exit 1
fi
bad_std_output="$(<"$WORK/bad_std.out")"
if [[ "$bad_std_output" != *"Invalid language standard -std=c++29"* ]]; then
  echo "bad_std: missing invalid -std diagnostic" >&2
  exit 1
fi
