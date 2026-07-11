// RUN: -std=c++20
namespace N {
struct Tag {};
inline namespace V {
struct Tag {};
}
}  // namespace N

void use_ambiguous_tag(void) {
  N::Tag tag{};
  (void)tag;
}
