// RUN: -std=c++20 -g
// EXPECT_EXIT: 0
// Debug info must not change what a program means: block address ranges stay
// out of the statements that analysis looks at, and every label the debug
// info names is emitted exactly once.

static int order[8];
static int order_count;
static void Record(int id) { order[order_count++] = id; }

// The block ends in a return, so its closing label follows dead code.
static int Clamp(int n) {
  int total = 0;
  for (int i = 0; i < n; i++) {
    int square = i * i;
    total += square;
  }
  if (n > 100) {
    int big = n;
    return big;
  }
  return total;
}

// Statements after a returning `if constexpr` branch are discarded.
template <class T>
int Kind() {
  if constexpr (sizeof(T) == 1) {
    return 1;
  }
  return T::missing_member;
}

constexpr int Folded(int n) {
  int sum = 0;
  {
    int twice = n * 2;
    sum += twice;
  }
  return sum;
}
static_assert(Folded(4) == 8);

struct Member {
  ~Member() { Record(2); }
};
struct Base {
  ~Base() { Record(3); }
};
// The base is destroyed after the destructor body and the members.
struct Derived : Base {
  Member member;
  ~Derived() { Record(1); }
};

struct Guard {
  int* hits;
  ~Guard() { ++*hits; }
};
static void Throws(int* hits) {
  {
    Guard guard{hits};
    throw 7;
  }
}

template <class T>
struct Pattern {
  T value;
};

template <class T>
struct Invalid {
  using Child = decltype(T::no_such_member);
};
Invalid<int>* invalid_ptr;

struct AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte01 {};
struct AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte02 {};
struct AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte03 {};
struct AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte04 {};
struct AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte05 {};
struct AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte06 {};
struct AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte07 {};
struct AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte08 {};
struct AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte09 {};
struct AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte10 {};
struct AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte11 {};
struct AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte12 {};
struct AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte13 {};
struct AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte14 {};
struct AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte15 {};
struct AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte16 {};

template <class... Ts>
int CountTypes() {
  int count = 0;
  {
    count = sizeof...(Ts);
  }
  return count;
}

int main() {
  if (Clamp(4) != 14 || Clamp(200) != 200) {
    return 1;
  }
  if (Kind<char>() != 1) {
    return 2;
  }
  { Derived d; }
  if (order_count != 3 || order[0] != 1 || order[1] != 2 || order[2] != 3) {
    return 3;
  }
  int hits = 0;
  try {
    Throws(&hits);
  } catch (int) {
  }
  if (hits != 1) {
    return 4;
  }
  auto add = [](int a, int b) {
    int sum = a + b;
    return sum;
  };
  if (add(2, 3) != 5) {
    return 5;
  }
  Pattern<int> pattern{6};
  if (pattern.value != 6 || invalid_ptr != nullptr) {
    return 6;
  }
  int types = CountTypes<
      AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte01,
      AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte02,
      AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte03,
      AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte04,
      AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte05,
      AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte06,
      AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte07,
      AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte08,
      AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte09,
      AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte10,
      AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte11,
      AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte12,
      AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte13,
      AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte14,
      AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte15,
      AVeryLongStructureNameUsedToBuildAMangledNameLongerThanOneKilobyte16>();
  if (types != 16) {
    return 7;
  }
  return 0;
}
