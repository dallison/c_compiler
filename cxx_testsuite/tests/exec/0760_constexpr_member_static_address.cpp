// RUN: -std=c++17
// EXPECT_EXIT: 0

struct Base {};

struct Full : Base {
  constexpr const Base* base() const { return this; }
};

struct Holder {
  const Base* pointer;
  constexpr Holder(const Base* value) : pointer(value) {}
};

const Full full{};
constexpr const Base* pointer = full.base();
constexpr Holder holder{full.base()};

int main() {
  return pointer == &full && holder.pointer == &full ? 0 : 1;
}
