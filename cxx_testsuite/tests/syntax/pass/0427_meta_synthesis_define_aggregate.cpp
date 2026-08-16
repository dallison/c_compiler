// RUN: -std=c++26

#include <meta>

using std::meta::data_member_spec;
using std::meta::define_aggregate;
using std::meta::is_data_member_spec;

struct synthesized;

constexpr auto x_spec = data_member_spec(^^int, {.name = "x"});
static_assert(is_data_member_spec(x_spec));

constexpr auto completed = define_aggregate(^^synthesized, {x_spec});
static_assert(completed == ^^synthesized);

static_assert(sizeof(synthesized) == sizeof(int));

int main() { return 0; }
