// RUN: -std=c++20
namespace N {
int value = 1;
inline namespace V {
int value = 2;
}
}  // namespace N

void use_ambiguous_value(void) {
  (void)N::value;
}
