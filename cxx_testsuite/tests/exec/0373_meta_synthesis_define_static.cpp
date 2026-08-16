// RUN: -std=c++26
// EXPECT_EXIT: 0

namespace meta_synth {
template <class T>
[[davecc::meta_intrinsic]] consteval const T* define_static_string(T&& r);
}

using meta_synth::define_static_string;

static const char* promoted = define_static_string("abc");

int main() {
  return (promoted[0] == 'a' && promoted[1] == 'b' && promoted[2] == 'c') ? 0 : 1;
}
