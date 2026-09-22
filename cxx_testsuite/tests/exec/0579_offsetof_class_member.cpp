// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <cstddef>

class Spec {
 public:
  char conv_;
  int width_;
  int precision_;

  int check() const {
    static_assert(offsetof(Spec, conv_) == 0, "");
    return (int)offsetof(Spec, conv_);
  }
};

struct Pod {
  char conv_;
  int width_;
};

int main() {
  Spec s;
  static_assert(offsetof(Pod, conv_) == 0, "");
  return s.check() == 0 && offsetof(Pod, conv_) == 0 ? 0 : 1;
}
