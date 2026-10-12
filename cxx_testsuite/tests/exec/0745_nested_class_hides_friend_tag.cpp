// RUN: -std=c++17
// EXPECT_EXIT: 0

class Owner {
 public:
  friend class Helper;

  class Helper {
   public:
    explicit Helper(int) {}
    class Nested;
  };

 private:
  int value = 17;
  friend class Helper::Nested;
};

class Owner::Helper::Nested {
 public:
  static int read(const Owner& owner) { return owner.value; }
};

int main() {
  Owner owner;
  return Owner::Helper::Nested::read(owner) == 17 ? 0 : 1;
}
