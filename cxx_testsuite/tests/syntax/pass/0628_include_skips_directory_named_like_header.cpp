// RUN: -std=c++20 -I"$SUITE_ROOT/include/include_skips_directory/first" -I"$SUITE_ROOT/include/include_skips_directory/second"

// first/skip_me.hpp is a directory, not a header; the search goes on to
// second/skip_me.hpp.
#include <skip_me.hpp>

static_assert(kFoundRealHeader == 1);

int main() { return 0; }
