// RUN: -std=c++26
// EXPECT_EXIT: 0

struct Superbase {
  int id = 7;
};

struct Common : Superbase {
  unsigned counter = 0;
};

struct Left : virtual Common {
  unsigned left = 1;

  constexpr const unsigned& get_counter() const {
    return Common::counter;
  }
};

struct Right : virtual Common {
  unsigned right = 2;

  constexpr const unsigned& get_counter() const {
    return Common::counter;
  }
};

struct Child : Left, Right {
  unsigned x = 3;
  unsigned y = 4;
};

constexpr int construct_and_destroy() {
  Child local{};
  return local.id + local.x;
}

constexpr Child child{};
static_assert(static_cast<const Common*>(
                  static_cast<const Left*>(&child)) ==
              static_cast<const Common*>(
                  static_cast<const Right*>(&child)));
static_assert(child.id == 7);
static_assert(child.left + child.right + child.x + child.y == 10);
static_assert(construct_and_destroy() == 10);

int main() {
  return static_cast<const Common*>(static_cast<const Left*>(&child)) ==
                     static_cast<const Common*>(
                         static_cast<const Right*>(&child)) &&
                 child.id == 7
             ? 0
             : 1;
}
