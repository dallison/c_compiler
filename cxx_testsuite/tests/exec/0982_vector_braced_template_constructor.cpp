// RUN: -std=c++17
// EXPECT_EXIT: 0
// A braced vector element list-initializes through a constructor template.
// `vector<Sub>{{"name", value}}` and `push_back({"k", value})` both need that
// constructor to be viable while ranking the initializer_list conversion.

#include <string>
#include <vector>

struct Sub {
  template <typename Value>
  Sub(std::string key, Value&& value)
      : key_(std::move(key)), n_(sizeof(value)) {}

  std::string key_;
  unsigned n_;
};

static std::string FieldName() {
  return "f";
}

int main() {
  std::vector<Sub> vars = {
      {"name", FieldName()},
      {"other", "y"},
  };
  vars.push_back({"oneof_name", std::string("z")});
  if (vars.size() != 3) {
    return 1;
  }
  if (vars[0].key_ != "name" || vars[1].key_ != "other" ||
      vars[2].key_ != "oneof_name") {
    return 2;
  }
  return 0;
}
