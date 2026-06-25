// RUN: -std=c++20

extern int declared_then_inline;
inline int declared_then_inline = 3;
extern int declared_then_inline;

inline static int internal_inline = 4;
static int internal_inline_use(void) {
  return internal_inline;
}

namespace inline_linkage_ns {
inline int value = 5;
extern int value;
}

struct InlineLinkageStaticMember {
  static const int const_integral = 2;
  static int out_of_class;
  inline static int in_class = 6;
  static constexpr int constexpr_member = 7;
};

int out_of_class = 99;
int InlineLinkageStaticMember::out_of_class = 8;

static_assert(InlineLinkageStaticMember::constexpr_member == 7,
              "constexpr static data member is inline");
static_assert(InlineLinkageStaticMember::const_integral == 2,
              "const integral static data member is a constant expression");

int use_inline_variables(void) {
  return declared_then_inline + internal_inline_use() +
         inline_linkage_ns::value + InlineLinkageStaticMember::in_class +
         InlineLinkageStaticMember::out_of_class + out_of_class;
}
