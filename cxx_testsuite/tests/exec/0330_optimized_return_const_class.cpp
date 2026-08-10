// RUN: -std=c++23 -O1
// EXPECT_EXIT: 0

#include <string>

struct owned_value {
  std::string text;

  explicit owned_value(const char* value) : text(value) {}
};

static owned_value copy_value(const owned_value& value) {
  return value;
}

int main() {
  owned_value original("preserved");
  owned_value copy = copy_value(original);
  if (original.text != "preserved") return 1;
  if (copy.text != "preserved") return 2;
  return 0;
}
