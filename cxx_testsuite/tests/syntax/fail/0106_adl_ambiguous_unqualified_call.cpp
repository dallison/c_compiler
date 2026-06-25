// RUN: -std=c++17
// EXPECT: Ambiguous overload for collide
namespace adl_ambiguous_left {
struct Left {
  int value;
};

int collide(Left left, int value);
}

namespace adl_ambiguous_right {
struct Right {
  int value;
};

int collide(adl_ambiguous_left::Left left, Right right);
}

namespace adl_ambiguous_left {
int collide(Left left, adl_ambiguous_right::Right right);
}

int main(void) {
  adl_ambiguous_left::Left left = {1};
  adl_ambiguous_right::Right right = {2};
  return collide(left, right);
}
