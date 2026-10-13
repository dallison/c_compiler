// RUN: -std=c++17
// EXPECT_EXIT: 0

union Choice {
  constexpr Choice(const int* value) : pointer(value) {}

  int integer;
  const int* pointer;
};

template <int N>
struct Table {
  Choice entries[N];
};

const int target = 0;
constexpr Choice choice{&target};
static_assert(choice.pointer == &target);

constexpr Table<1> table = {{{&target}}};
static_assert(table.entries[0].pointer == &target);

int main() {
  if (choice.pointer != &target) {
    return 1;
  }
  if (table.entries[0].pointer != &target) {
    return 2;
  }
  return 0;
}
