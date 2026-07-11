// RUN: -std=c++20
namespace N {
namespace Child {}
inline namespace V {
namespace Child {}
}
}  // namespace N

void use_ambiguous_child(void) {
  using namespace N::Child;
}
