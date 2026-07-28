// RUN: -std=c++23
// EXPECT-ASM: _ZNH5value3addER5valuei
// EXPECT-ASM: _ZNH5value4copyE5valuei

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
