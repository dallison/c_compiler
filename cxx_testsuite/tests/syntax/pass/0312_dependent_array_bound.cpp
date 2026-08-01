// RUN: -std=c++20

template <unsigned long Capacity>
struct storage {
  int values[Capacity == 0 ? 1 : Capacity];
};

static_assert(sizeof(storage<0>) == sizeof(int));
static_assert(sizeof(storage<4>) == 4 * sizeof(int));

int main() {
  storage<3> value{};
  value.values[2] = 7;
  return value.values[2] != 7;
}
