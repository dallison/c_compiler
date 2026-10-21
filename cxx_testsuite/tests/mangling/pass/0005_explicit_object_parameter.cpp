// RUN: -std=c++23
// EXPECT-ASM: _ZNH5value3addERS_i
// EXPECT-ASM: _ZNH5value4copyES_i

struct value {
  int number;

  int add(this value& self, int amount) {
    return self.number + amount;
  }

  int copy(this value self, int amount) {
    return self.number + amount;
  }
};

int use(value& object) {
  return object.add(1) + object.copy(2);
}
