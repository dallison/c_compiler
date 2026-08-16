// RUN: -std=c++26

[[= ^^int ]] constexpr int annotated = 0;

static_assert(annotated == 0);
