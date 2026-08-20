// RUN: -std=c++29
// EXPECT_EXIT: 0

struct counter {
  int value;

  counter& operator++() {
    ++value;
    return *this;
  }

  counter& operator--() {
    --value;
    return *this;
  }

  counter operator++(int) = default;
  counter operator--(int) = default;
};

struct explicit_object_counter {
  int value;

  explicit_object_counter& operator++() {
    ++value;
    return *this;
  }

  explicit_object_counter operator++(
      this explicit_object_counter&, int) = default;
};

struct out_of_class_counter {
  int value;

  out_of_class_counter& operator++() {
    ++value;
    return *this;
  }

  out_of_class_counter operator++(int);
};

out_of_class_counter out_of_class_counter::operator++(int) = default;

struct free_counter {
  int value;

  free_counter& operator++() {
    ++value;
    return *this;
  }
};

free_counter operator++(free_counter&, int) = default;

class hidden_friend_counter {
 public:
  explicit hidden_friend_counter(int value) : value_(value) {}

  hidden_friend_counter& operator++() {
    ++value_;
    return *this;
  }

  int value() const {
    return value_;
  }

  friend hidden_friend_counter operator++(hidden_friend_counter&, int) =
      default;

 private:
  int value_;
};

template <typename T>
struct template_counter {
  T value;

  template_counter& operator++() {
    ++value;
    return *this;
  }

  template_counter operator++(int) = default;
};

int main() {
  counter first{4};
  counter before_increment = first++;
  if (before_increment.value != 4 || first.value != 5) {
    return 1;
  }
  counter before_decrement = first--;
  if (before_decrement.value != 5 || first.value != 4) {
    return 2;
  }

  explicit_object_counter second{7};
  explicit_object_counter before_explicit = second++;
  if (before_explicit.value != 7 || second.value != 8) {
    return 3;
  }

  out_of_class_counter third{8};
  out_of_class_counter before_out_of_class = third++;
  if (before_out_of_class.value != 8 || third.value != 9) {
    return 4;
  }

  free_counter fourth{9};
  free_counter before_free = fourth++;
  if (before_free.value != 9 || fourth.value != 10) {
    return 5;
  }

  hidden_friend_counter fifth{10};
  hidden_friend_counter before_hidden_friend = fifth++;
  if (before_hidden_friend.value() != 10 || fifth.value() != 11) {
    return 6;
  }

  template_counter<long> sixth{11};
  template_counter<long> before_template = sixth++;
  if (before_template.value != 11 || sixth.value != 12) {
    return 7;
  }

  return 0;
}
