// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <any>
#include <exception>
#include <initializer_list>
#include <typeinfo>

struct Small {
  static int alive;
  static int copies;
  static int moves;

  int value;

  Small(int initial) : value(initial) { alive++; }
  Small(const Small& other) : value(other.value) {
    alive++;
    copies++;
  }
  Small(Small&& other) noexcept : value(other.value) {
    alive++;
    moves++;
    other.value = -1;
  }
  ~Small() { alive--; }
};

int Small::alive;
int Small::copies;
int Small::moves;

struct Large {
  static int alive;
  int values[32];

  Large(int value) {
    alive++;
    for (int i = 0; i < 32; i++) values[i] = value + i;
  }
  Large(const Large& other) {
    alive++;
    for (int i = 0; i < 32; i++) values[i] = other.values[i];
  }
  Large(Large&& other) noexcept {
    alive++;
    for (int i = 0; i < 32; i++) values[i] = other.values[i];
  }
  ~Large() { alive--; }
};

int Large::alive;

struct FromList {
  int total;

  FromList(std::initializer_list<int> values, int extra) : total(extra) {
    for (const int* it = values.begin(); it != values.end(); ++it) {
      total += *it;
    }
  }
};

struct ConstructionError {
};

struct ThrowsDuringConstruction {
  ThrowsDuringConstruction(int) { throw ConstructionError(); }
  ThrowsDuringConstruction(const ThrowsDuringConstruction&) = default;
};

struct ThrowsDuringCopy {
  static bool throw_now;
  int value;

  ThrowsDuringCopy(int initial) : value(initial) {}
  ThrowsDuringCopy(const ThrowsDuringCopy& other) : value(other.value) {
    if (throw_now) throw ConstructionError();
  }
  ThrowsDuringCopy(ThrowsDuringCopy&& other) noexcept : value(other.value) {}
};

bool ThrowsDuringCopy::throw_now;

struct MoveObserved {
  int value;
  MoveObserved(int initial) : value(initial) {}
  MoveObserved(const MoveObserved&) = default;
  MoveObserved(MoveObserved&& other) noexcept : value(other.value) {
    other.value = -1;
  }
};

int main() {
  std::any empty;
  if (empty.has_value()) return 1;
  if (empty.type() != typeid(void)) return 2;
  if (std::any_cast<int>(&empty) != nullptr) return 3;
  if (std::any_cast<int>(static_cast<const std::any*>(&empty)) != nullptr)
    return 4;

  std::any integer = 41;
  if (!integer.has_value() || integer.type() != typeid(int)) return 5;
  int* integer_pointer = std::any_cast<int>(&integer);
  if (integer_pointer == nullptr || *integer_pointer != 41) return 6;
  *integer_pointer = 42;
  if (std::any_cast<int>(integer) != 42) return 7;
  int& integer_reference = std::any_cast<int&>(integer);
  integer_reference = 43;
  const std::any& const_integer = integer;
  const int& const_reference = std::any_cast<const int&>(const_integer);
  if (const_reference != 43) return 8;
  if (std::any_cast<long>(&integer) != nullptr) return 9;

  bool caught_bad_any = false;
  try {
    (void)std::any_cast<long>(integer);
  } catch (const std::bad_any_cast& error) {
    caught_bad_any = error.what() != nullptr;
  }
  if (!caught_bad_any) return 10;

  bool caught_bad_cast = false;
  try {
    (void)std::any_cast<long>(integer);
  } catch (const std::bad_cast&) {
    caught_bad_cast = true;
  }
  if (!caught_bad_cast) return 11;

  bool caught_exception = false;
  try {
    (void)std::any_cast<long>(integer);
  } catch (const std::exception&) {
    caught_exception = true;
  }
  if (!caught_exception) return 12;

  Small::alive = Small::copies = Small::moves = 0;
  {
    std::any small(std::in_place_type<Small>, 17);
    if (Small::alive != 1) return 13;
    std::any small_copy = small;
    if (Small::alive != 2 || Small::copies != 1) return 14;
    std::any small_move = static_cast<std::any&&>(small_copy);
    if (small_copy.has_value() || Small::alive != 2 || Small::moves != 1)
      return 15;
    if (std::any_cast<Small&>(small_move).value != 17) return 16;
    small_move = small_move;
    if (std::any_cast<Small&>(small_move).value != 17) return 17;
    if (Small::alive != 2) return 35;
    small_move = static_cast<std::any&&>(small_move);
    if (std::any_cast<Small&>(small_move).value != 17) return 18;
    if (Small::alive != 2) return 36;
    small.reset();
    if (Small::alive != 1) return 37;
    small_move.reset();
    if (Small::alive != 0) return 38;
  }
  if (Small::alive != 0) return 19;

  Large::alive = 0;
  {
    std::any large(std::in_place_type<Large>, 100);
    std::any copy = large;
    if (Large::alive != 2) return 20;
    if (std::any_cast<Large&>(copy).values[31] != 131) return 21;
    std::any moved = static_cast<std::any&&>(copy);
    if (copy.has_value() || Large::alive != 2) return 22;
    if (std::any_cast<Large&>(moved).values[7] != 107) return 23;
    moved.reset();
    if (moved.has_value()) return 39;
    if (Large::alive != 1) return 40;
  }
  if (Large::alive != 0) return 25;

  std::any first = 5;
  std::any second = 9L;
  first.swap(second);
  if (std::any_cast<long>(first) != 9L ||
      std::any_cast<int>(second) != 5)
    return 26;
  swap(first, second);
  if (std::any_cast<int>(first) != 5 ||
      std::any_cast<long>(second) != 9L)
    return 27;

  FromList& list =
      first.emplace<FromList>({1, 2, 3}, 4);
  if (list.total != 10 || std::any_cast<FromList&>(first).total != 10)
    return 28;

  std::any made = std::make_any<Small>(29);
  if (std::any_cast<Small&>(made).value != 29) return 29;
  std::any made_list = std::make_any<FromList>({4, 5}, 6);
  if (std::any_cast<FromList&>(made_list).total != 15) return 30;

  std::any decayed(std::in_place_type<const int>, 70);
  if (decayed.type() != typeid(int) || std::any_cast<int>(decayed) != 70)
    return 47;
  int& decayed_reference = decayed.emplace<const int>(71);
  if (decayed_reference != 71 || std::any_cast<int>(decayed) != 71)
    return 48;

  bool construction_threw = false;
  try {
    made.emplace<ThrowsDuringConstruction>(1);
  } catch (const ConstructionError&) {
    construction_threw = true;
  }
  if (!construction_threw || made.has_value()) return 31;

  bool constructor_threw = false;
  try {
    std::any failed(std::in_place_type<ThrowsDuringConstruction>, 2);
  } catch (const ConstructionError&) {
    constructor_threw = true;
  }
  if (!constructor_threw) return 43;

  ThrowsDuringCopy::throw_now = false;
  std::any copy_source = ThrowsDuringCopy(55);
  std::any copy_destination = 66;
  ThrowsDuringCopy::throw_now = true;
  bool copy_threw = false;
  try {
    copy_destination = copy_source;
  } catch (const ConstructionError&) {
    copy_threw = true;
  }
  ThrowsDuringCopy::throw_now = false;
  if (!copy_threw || std::any_cast<int>(copy_destination) != 66) return 44;

  Large::alive = 0;
  std::any no_value;
  std::any heap_value(std::in_place_type<Large>, 200);
  no_value.swap(heap_value);
  if (!no_value.has_value() || heap_value.has_value() ||
      std::any_cast<Large&>(no_value).values[3] != 203 ||
      Large::alive != 1)
    return 45;
  swap(no_value, heap_value);
  if (no_value.has_value() || !heap_value.has_value() ||
      std::any_cast<Large&>(heap_value).values[9] != 209 ||
      Large::alive != 1)
    return 46;
  heap_value.reset();
  if (Large::alive != 0) return 47;

  std::any move_value(std::in_place_type<MoveObserved>, 77);
  MoveObserved moved_out =
      std::any_cast<MoveObserved>(static_cast<std::any&&>(move_value));
  if (moved_out.value != 77) return 41;
  if (std::any_cast<MoveObserved&>(move_value).value != -1) return 42;

  integer = nullptr;
  if (integer.type() != typeid(std::nullptr_t) ||
      std::any_cast<std::nullptr_t>(&integer) == nullptr)
    return 33;
  integer.reset();
  if (integer.has_value()) return 34;
  return 0;
}
