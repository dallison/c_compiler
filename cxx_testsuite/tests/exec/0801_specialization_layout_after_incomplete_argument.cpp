// RUN: -std=c++17
// EXPECT_EXIT: 0

// A class template specialization named while its argument is only
// forward-declared is laid out once the argument's definition is complete:
// direct, array, nested-class, base-class, and nested-specialization members
// all get their full storage, so neighbouring objects are not overwritten.

#include <optional>
#include <utility>

template <class B> struct Arr { char tag; B items[2]; };
template <class B> struct Nest { struct In { B b; int z; }; In in; char c; };
template <class B> struct Derived : B { char extra; };
template <class B> struct Outer { std::pair<char, B> p; char tail; };

struct Record;
using Entry = std::pair<const int, Record>;
using A1 = Arr<Record>;
using N1 = Nest<Record>;
using D1 = Derived<Record>;
using O1 = Outer<Record>;
using Opt = std::optional<Record>;

struct Record { long value = 0; };

int main() {
  static_assert(sizeof(Entry) == 16);
  static_assert(sizeof(A1) == 24);
  static_assert(sizeof(N1) == 24);
  static_assert(sizeof(D1) == 16);
  static_assert(sizeof(O1) == 24);
  static_assert(sizeof(Opt) == 16);

  Entry entry{1, Record{}};
  A1 arr{};
  D1 derived{};
  Opt opt(Record{});
  int guard = 9;
  entry.second.value = -1;
  arr.items[1].value = -1;
  derived.value = -1;
  derived.extra = 'x';
  opt->value = -1;
  if (guard != 9) {
    return 1;
  }
  if (entry.second.value != -1 || arr.items[1].value != -1 ||
      derived.value != -1 || opt->value != -1) {
    return 2;
  }
  return 0;
}
