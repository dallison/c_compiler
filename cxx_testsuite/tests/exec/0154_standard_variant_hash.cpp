// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <functional>
#include <variant>

int main(void) {
  std::hash<std::monostate> monostate_hash;
  if (monostate_hash(std::monostate{}) != monostate_hash(std::monostate{})) {
    return 1;
  }

  std::variant<int, long> one(7);
  std::variant<int, long> another_one(7);
  std::variant<int, long> different_value(8);
  std::variant<int, long> different_index(std::in_place_index<1>, 7L);

  std::hash<std::variant<int, long> > variant_hash;
  size_t one_hash = variant_hash(one);
  if (one_hash != variant_hash(another_one)) {
    return 2;
  }
  if (one_hash == variant_hash(different_value)) {
    return 3;
  }
  if (one_hash == variant_hash(different_index)) {
    return 4;
  }

  const char* text = "hello";
  std::variant<int, const char*> pointer_value(text);
  std::variant<int, const char*> same_pointer(text);
  std::hash<std::variant<int, const char*> > pointer_hash;
  if (pointer_hash(pointer_value) != pointer_hash(same_pointer)) {
    return 5;
  }

  std::variant<std::monostate, int> empty;
  std::hash<std::variant<std::monostate, int> > empty_hash;
  if (empty_hash(empty) != empty_hash(empty)) {
    return 6;
  }

  return 0;
}
