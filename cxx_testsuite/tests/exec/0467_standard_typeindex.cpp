// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <typeindex>
#include <unordered_map>

struct Base {
  virtual ~Base() {}
};

struct Derived : Base {};

int main() {
  std::type_index integer(typeid(int));
  std::type_index same_integer(typeid(int));
  std::type_index character(typeid(char));

  if (integer != same_integer || !(integer == same_integer)) return 1;
  if (integer == character || integer.name() == nullptr) return 2;
  if (integer.hash_code() != typeid(int).hash_code()) return 3;
  if ((integer < character) != typeid(int).before(typeid(char))) return 4;
  if ((integer <= character) != !(character < integer)) return 5;
  if ((integer > character) != (character < integer)) return 6;
  if ((integer >= character) != !(integer < character)) return 7;

  std::hash<std::type_index> hasher;
  if (hasher(integer) != hasher(same_integer)) return 8;

#ifndef __6502__
  std::unordered_map<std::type_index, int> values;
  values[integer] = 11;
  values[character] = 17;
  if (values[std::type_index(typeid(int))] != 11 ||
      values[std::type_index(typeid(char))] != 17)
    return 9;
#endif

  Derived object;
  Base& base = object;
  std::type_index dynamic(typeid(base));
  if (dynamic != std::type_index(typeid(Derived))) return 10;

  same_integer = character;
  if (same_integer != character) return 11;
  return 0;
}
