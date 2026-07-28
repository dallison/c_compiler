// RUN: -std=c++23
// EXPECT_EXIT: 0

struct value {
  int number;

  int add(this value& self, int amount) {
    return self.number + amount;
  }

  int read(this const value& self) {
    return self.number;
  }

  int category(this value&) {
    return 1;
  }

  int category(this value&&) {
    return 2;
  }

  template <class Self>
  int forward_read(this Self&& self) {
    return self.number;
  }

  int abbreviated(this auto&& self) {
    return self.number;
  }

  int by_value(this value self) {
    ++self.number;
    return self.number;
  }
};

template <class T>
struct box {
  T item;

  T get(this const box& self) {
    return self.item;
  }

  template <class Self>
  T get_forward(this Self&& self) {
    return self.item;
  }
};

struct base {
  int number;

  int base_read(this base& self) {
    return self.number;
  }

  template <class Self>
  int shadow(this Self&& self) {
    return self.number;
  }
};

struct derived : base {
  int number;
};

struct out_of_line {
  int number;
  int add(this out_of_line& self, int amount);
};

int out_of_line::add(this out_of_line& self, int amount) {
  return self.number + amount;
}

int main() {
  value object{10};
  const value constant{20};
  value* pointer = &object;
  if (object.add(1) != 11 || pointer->add(2) != 12 ||
      constant.read() != 20) {
    return 1;
  }
  if (object.category() != 1 || value{0}.category() != 2) {
    return 2;
  }
  if (object.forward_read() != 10 || value{30}.forward_read() != 30) {
    return 3;
  }
  if (object.abbreviated() != 10 || value{31}.abbreviated() != 31) {
    return 10;
  }
  if (object.by_value() != 11 || object.number != 10) {
    return 4;
  }
  int (*function)(value&, int) = &value::add;
  if (function(object, 3) != 13) {
    return 5;
  }
  box<int> boxed{40};
  if (boxed.get() != 40 || boxed.get_forward() != 40) {
    return 6;
  }
  derived child;
  base& parent = child;
  parent.number = 45;
  child.number = 50;
  if (child.base_read() != 45 || child.shadow() != 50) {
    return 7;
  }
  auto fibonacci = [](this auto self, int n) -> int {
    return n < 2 ? n : self(n - 1) + self(n - 2);
  };
  if (fibonacci(7) != 13) {
    return 8;
  }
  out_of_line separate{60};
  if (separate.add(4) != 64) {
    return 9;
  }
  return 0;
}
