// RUN: -std=c++23

struct value {
  int number;

  constexpr int read(this const value& self) {
    return self.number;
  }

  constexpr int add(this value& self, int amount) {
    return self.number + amount;
  }

  constexpr int category(this value&) {
    return 1;
  }

  constexpr int category(this value&&) {
    return 2;
  }

  template <class Self>
  constexpr int forward_read(this Self&& self) {
    return self.number;
  }

  constexpr int abbreviated(this auto&& self) {
    return self.number;
  }
};

template <class T>
struct box {
  T item;

  constexpr T get(this const box& self) {
    return self.item;
  }

  template <class Self>
  constexpr T get_forward(this Self&& self) {
    return self.item;
  }
};

struct out_of_line {
  int number;
  constexpr int read(this const out_of_line& self);
};

constexpr int out_of_line::read(this const out_of_line& self) {
  return self.number;
}

static_assert(value{3}.read() == 3);
static_assert(value{4}.category() == 2);
static_assert(value{5}.forward_read() == 5);
static_assert(value{6}.abbreviated() == 6);
static_assert(box<int>{7}.get() == 7);
static_assert(box<int>{8}.get_forward() == 8);
static_assert(out_of_line{9}.read() == 9);

int main() {
  value object{9};
  int (*function)(value&, int) = &value::add;
  return function(object, 1);
}
