// RUN: -std=c++26
// EXPECT: constexpr conversion from void pointer requires an object of similar type

constexpr bool wrong_type_round_trip() {
  int value = 0;
  void* erased = &value;
  long* wrong = static_cast<long*>(erased);
  return wrong != nullptr;
}

static_assert(wrong_type_round_trip());
