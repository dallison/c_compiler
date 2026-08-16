// RUN: -std=c++26

// Legacy query-semantics coverage now lives in 0427_reflection_meta_public_header.cpp
// which uses the standard <meta> header directly.

#include <meta>

using namespace std::meta;

static_assert(is_type(^^int));

int main() { return 0; }
