// RUN: -std=c++29 -fcontracts=observe
// EXPECT_EXIT: 0

static int sequence;

static bool record(int digit, bool value) {
  sequence = sequence * 10 + digit;
  return value;
}

struct base {
  virtual int value(const int input)
      pre (record(1, input == 7))
      post (result: record(5, result == input + 1)) {
    sequence = sequence * 10 + 9;
    return input + 100;
  }
};

struct derived : base {
  int value(const int input) override
      pre (record(2, input == 7))
      post (result: record(4, result == input + 1)) {
    sequence = sequence * 10 + 3;
    return input + 1;
  }

  int base_value(const int input) {
    return base::value(input);
  }
};

struct reference_base {
  virtual const int& select(const int& input)
      pre (record(1, input == 11))
      post (result: record(5, &result == &input)) {
    return input;
  }
};

struct reference_derived : reference_base {
  const int& select(const int& input) override
      pre (record(2, input == 11))
      post (result: record(4, &result == &input)) {
    sequence = sequence * 10 + 3;
    return input;
  }
};

int main() {
  derived object;
  base& interface = object;
  int result = interface.value(7);
  if (result != 8 || sequence != 12345) {
    return 1;
  }

  sequence = 0;
  result = object.value(7);
  if (result != 8 || sequence != 234) {
    return 2;
  }

  sequence = 0;
  result = object.base_value(7);
  if (result != 107 || sequence != 195) {
    return 3;
  }

  sequence = 0;
  int (base::*member)(const int) = &base::value;
  result = (object.*member)(7);
  if (result != 8 || sequence != 234) {
    return 4;
  }

  sequence = 0;
  int referenced = 11;
  reference_derived reference_object;
  reference_base& reference_interface = reference_object;
  const int& selected = reference_interface.select(referenced);
  return &selected == &referenced && sequence == 12345 ? 0 : 5;
}
