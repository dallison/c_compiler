// RUN: -std=c++29

struct counter {
  int value;

  constexpr counter& operator++() {
    ++value;
    return *this;
  }

  constexpr counter& operator--() {
    --value;
    return *this;
  }

  constexpr counter operator++(int) = default;
  constexpr counter operator--(int) = default;
};

constexpr bool test_member_postfix() {
  counter value{4};
  counter before_increment = value++;
  if (before_increment.value != 4 || value.value != 5) {
    return false;
  }
  counter before_decrement = value--;
  return before_decrement.value == 5 && value.value == 4;
}

static_assert(test_member_postfix());

struct explicit_object_counter {
  int value;

  constexpr explicit_object_counter& operator++() {
    ++value;
    return *this;
  }

  constexpr explicit_object_counter operator++(
      this explicit_object_counter&, int) = default;
};

constexpr bool test_explicit_object_postfix() {
  explicit_object_counter value{7};
  explicit_object_counter before = value++;
  return before.value == 7 && value.value == 8;
}

static_assert(test_explicit_object_postfix());

struct out_of_class_counter {
  int value;

  constexpr out_of_class_counter& operator++() {
    ++value;
    return *this;
  }

  constexpr out_of_class_counter operator++(int);
};

constexpr out_of_class_counter out_of_class_counter::operator++(int) =
    default;

constexpr bool test_out_of_class_postfix() {
  out_of_class_counter value{8};
  out_of_class_counter before = value++;
  return before.value == 8 && value.value == 9;
}

static_assert(test_out_of_class_postfix());

struct free_counter {
  int value;

  constexpr free_counter& operator++() {
    ++value;
    return *this;
  }
};

constexpr free_counter operator++(free_counter&, int) = default;

constexpr bool test_free_postfix() {
  free_counter value{9};
  free_counter before = value++;
  return before.value == 9 && value.value == 10;
}

static_assert(test_free_postfix());

enum count_value {
  count_zero,
  count_one,
  count_two,
};

constexpr count_value& operator++(count_value& value) {
  value = static_cast<count_value>(static_cast<int>(value) + 1);
  return value;
}

constexpr count_value operator++(count_value&, int) = default;

constexpr bool test_enum_postfix() {
  count_value value = count_zero;
  count_value before = value++;
  return before == count_zero && value == count_one;
}

static_assert(test_enum_postfix());

struct unavailable_prefix {
  unavailable_prefix operator++(int) = default;
};

struct unavailable_copy {
  unavailable_copy() = default;
  unavailable_copy(const unavailable_copy&) = delete;

  unavailable_copy& operator++() {
    return *this;
  }

  unavailable_copy operator++(int) = default;
};

class inaccessible_copy {
 public:
  inaccessible_copy() = default;

  inaccessible_copy& operator++() {
    return *this;
  }

 private:
  inaccessible_copy(const inaccessible_copy&) = default;
};

inaccessible_copy operator++(inaccessible_copy&, int) = default;

class hidden_friend_counter {
 public:
  constexpr explicit hidden_friend_counter(int value) : value_(value) {}

  constexpr hidden_friend_counter& operator++() {
    ++value_;
    return *this;
  }

  constexpr int value() const {
    return value_;
  }

  friend constexpr hidden_friend_counter operator++(
      hidden_friend_counter&, int) = default;

  constexpr hidden_friend_counter(const hidden_friend_counter&) = default;

 private:
  int value_;
};

constexpr bool test_hidden_friend_postfix() {
  hidden_friend_counter value{13};
  hidden_friend_counter before = value++;
  return before.value() == 13 && value.value() == 14;
}

static_assert(test_hidden_friend_postfix());

class deleted_hidden_friend_counter {
  friend deleted_hidden_friend_counter operator++(
      deleted_hidden_friend_counter&, int) = default;

 public:
  deleted_hidden_friend_counter& operator++() {
    return *this;
  }

 private:
  deleted_hidden_friend_counter(
      const deleted_hidden_friend_counter&) = delete;
};

template <typename T>
struct template_counter {
  T value;

  constexpr template_counter& operator++() {
    ++value;
    return *this;
  }

  constexpr template_counter operator++(int) = default;
};

constexpr bool test_class_template_postfix() {
  template_counter<int> value{11};
  template_counter<int> before = value++;
  return before.value == 11 && value.value == 12;
}

static_assert(test_class_template_postfix());
