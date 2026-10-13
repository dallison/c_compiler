// RUN: -std=c++17
// EXPECT_EXIT: 0

struct Tag {};

extern const unsigned data[];

union Value {
  constexpr Value(Tag, const unsigned* pointer) : pointer(pointer) {}

  int integer;
  const unsigned* pointer;
};

struct Entry {
  constexpr Entry(Value value) : value(value) {}

  Value value;
};

constexpr Entry entry{{Tag{}, data}};
static_assert(entry.value.pointer == data);

const unsigned data[] = {42};

int main() {
  return entry.value.pointer == data ? 0 : 1;
}
