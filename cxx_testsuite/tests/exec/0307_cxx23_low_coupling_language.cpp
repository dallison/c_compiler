// RUN: -std=c++23
// EXPECT_EXIT: 0

struct move_only {
  int value;

  explicit move_only(int input) : value(input) {}
  move_only(const move_only&) = delete;
  move_only(move_only&& other) : value(other.value) {
    other.value = 0;
  }
};

struct copy_or_move {
  int value;

  explicit copy_or_move(int input) : value(input) {}
  copy_or_move(const copy_or_move& other) : value(other.value + 10) {}
  copy_or_move(copy_or_move&& other) : value(other.value + 20) {
    other.value = 0;
  }
};

int guard_destructions = 0;

struct loop_guard {
  ~loop_guard() {
    ++guard_destructions;
  }
};

move_only return_parameter(move_only value) {
  return value;
}

move_only&& return_rvalue_reference(move_only&& value) {
  return value;
}

copy_or_move return_const_local() {
  const copy_or_move value(5);
  return value;
}

int main() {
  if (sizeof(1uz) != sizeof(void*) || 0z - 1 >= 0) {
    return 1;
  }

  auto add = [](int left, int right) static { return left + right; };
  if (add(20, 22) != 42) {
    return 2;
  }

  int sum = 0;
  if (using value_type = int; value_type value = 3) {
    sum += value;
  }
  switch (using value_type = int; value_type value = 4) {
    case 4:
      sum += value;
      break;
  }
  for (using value_type = int; sum < 8;) {
    value_type increment = 1;
    sum += increment;
  }
  int values[2] = {5, 6};
  for (using value_type = int; value_type value : values) {
    sum += value;
  }
  if (sum != 19) {
    return 3;
  }
  for (loop_guard guard; int value : values) {
    sum += value;
  }
  if (guard_destructions != 1 || sum != 30) {
    return 8;
  }

  int reached = 0;
  {
    goto trailing;
    reached = 1;
trailing:
  }
  if (reached != 0) {
    return 4;
  }

  move_only original(7);
  move_only moved = return_parameter(static_cast<move_only&&>(original));
  if (original.value != 0 || moved.value != 7) {
    return 5;
  }
  move_only reference_source(8);
  move_only&& reference =
      return_rvalue_reference(static_cast<move_only&&>(reference_source));
  if (&reference != &reference_source || reference.value != 8) {
    return 6;
  }

  copy_or_move copied = return_const_local();
  if (copied.value != 5 && copied.value != 15) {
    return 7;
  }

  return 0;
}
