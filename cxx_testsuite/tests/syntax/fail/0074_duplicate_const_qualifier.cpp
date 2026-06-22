// RUN: -std=c++20

const const int duplicate_const = 42;

static_assert(duplicate_const == 42,
              "duplicate const qualifiers are invalid");
