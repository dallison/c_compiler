// RUN: -std=c++26
// EXPECT: unevaluated string in static_assert cannot have an encoding prefix

static_assert(true, u8"message");
