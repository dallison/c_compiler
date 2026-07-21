// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0

#include <map>
#include <string>
#include <utility>

class PrivateValue {
 public:
  explicit PrivateValue(int value) : value_(value) {}

  int value() const { return value_; }

  friend int friend_value(const PrivateValue& value) {
    return value.value_;
  }

 private:
  int value_;
};

struct ReferencedValue {
  int value;
};

inline int read_reference(const ReferencedValue& value) {
  return value.value;
}

// This function is compiled but not executed. Its postfix increment body
// contains a template initializer that must keep the inliner from cloning an
// unresolved body.
void compile_map_postincrement(std::map<int, int>::iterator& iterator) {
  iterator++;
}

[[gnu::noinline]] int add_one(int value) {
  return value + 1;
}

[[gnu::noinline]] int preserve_across_call(int a, int b, int c, int d, int e) {
  int called = add_one(40);
  return a + b + c + d + e + called;
}

int main() {
  PrivateValue private_value(7);
  if (private_value.value() != 7 || friend_value(private_value) != 7) {
    return 1;
  }

  ReferencedValue referenced_value{11};
  if (read_reference(referenced_value) != 11) {
    return 2;
  }

  std::string source("moved");
  std::string destination = std::move(source);
  if (destination != "moved") {
    return 3;
  }

  if (preserve_across_call(1, 2, 3, 4, 5) != 56) {
    return 4;
  }

  return 0;
}
