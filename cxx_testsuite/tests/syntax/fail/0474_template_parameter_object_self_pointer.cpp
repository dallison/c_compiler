// RUN: -std=c++26

struct SelfPointer {
  SelfPointer* pointer;

  constexpr SelfPointer() : pointer(this) {}
};

template <SelfPointer object>
struct Holder {};

Holder<SelfPointer{}> invalid;
