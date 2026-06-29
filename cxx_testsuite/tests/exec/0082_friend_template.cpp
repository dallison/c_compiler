// RUN: -std=c++20
// EXPECT_EXIT: 0
//
// Friendship declared in a class template is granted by every instantiation.

template <typename T>
class Box {
  T value;

 public:
  Box(T v) : value(v) {}
  friend class Opener;
};

class Opener {
 public:
  template <typename T>
  T open(const Box<T>& b) const {
    return b.value;
  }
};

int main(void) {
  Box<int> bi(5);
  Box<long> bl(9);
  Opener o;
  if (o.open(bi) != 5) {
    return 1;
  }
  if (o.open(bl) != 9) {
    return 2;
  }
  return 0;
}
