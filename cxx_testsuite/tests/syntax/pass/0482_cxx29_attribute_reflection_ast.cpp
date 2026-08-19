// RUN: -std=c++29

#include <meta>

struct [[nodiscard]] item {};

constexpr auto attribute = ^^[[nodiscard]];

static_assert(std::meta::is_attribute(attribute));

int main() { return 0; }
