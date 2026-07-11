// RUN: -std=c++20
namespace N {
inline namespace V {
int value = 1;
struct Tag {};
}
}

namespace N {
inline namespace Inner {
inline namespace Deep {
int nested_value = 2;
}
}
}

namespace N::inline Mid {
int mid_value = 3;
}

namespace N::inline Mid::inline Leaf {
int leaf_value = 4;
}

inline namespace GlobalInline {
int global_value = 5;
}

void use_enclosing_qualified(void) {
  (void)N::value;
  (void)N::nested_value;
  (void)N::mid_value;
  (void)N::leaf_value;
  (void)N::Deep::nested_value;
}

void use_using_directive(void) {
  using namespace N;
  (void)value;
  (void)nested_value;
  (void)mid_value;
  (void)leaf_value;
}

void use_tags(void) {
  N::Tag tag{};
  (void)tag;
}

namespace reopen {
inline namespace sticky {}
namespace sticky {
int reopened = 6;
}
}  // namespace reopen

namespace specialization_owner {
inline namespace version {
template <class T>
struct Box {};
}

template <>
struct Box<int> {
  int value;
};
}  // namespace specialization_owner

int main(void) {
  use_enclosing_qualified();
  use_using_directive();
  use_tags();
  (void)global_value;
  (void)reopen::reopened;
  specialization_owner::Box<int> box{};
  (void)box;
  return N::value + N::nested_value + N::mid_value + N::leaf_value + global_value;
}
