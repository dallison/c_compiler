// RUN: -std=c++26

#include <meta>

using std::define_static_string;
using std::is_string_literal;
using std::meta::reflect_constant;

constexpr const char* promoted = define_static_string("abc");

static_assert(reflect_constant(42) == reflect_constant(42));
static_assert(is_string_literal("hello"));
static_assert(is_string_literal(define_static_string("abc")));
static_assert(define_static_string("abc")[0] == 'a');
static_assert(promoted[1] == 'b');
static_assert(!is_string_literal(+""));

int main() { return 0; }
