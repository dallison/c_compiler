// RUN: -std=c++20 -fconstexpr-eval=pcode

// Brace elision leaves a namespace-scope array of aggregates with one
// designated entry per scalar (`[0].name = "US-ASCII"`), and a constructor
// taking `const T&` binds that reference to a materialized scalar temporary.

struct Encoding {
  int id;
  const char* name;
  int offset, count;
};

inline constexpr Encoding encodings[] = {{3, "US-ASCII", 0, 10},
                                         {106, "UTF-8", 1, 2}};
static_assert(encodings[0].id == 3 && encodings[0].count == 10);
static_assert(encodings[1].id == 106 && encodings[1].offset == 1);
static_assert(encodings[0].name[0] == 'U' && encodings[1].name[4] == '8');

struct Inner {
  int a;
  int b;
};
struct Outer {
  Inner inner;
  int z;
};

constexpr Outer outers[] = {{{8, 4}, 9}, {{1, 2}, 3}};
static_assert(outers[0].inner.a == 8 && outers[0].inner.b == 4);
static_assert(outers[0].z == 9 && outers[1].inner.b == 2);

union Number {
  int i;
  float f;
};

constexpr Number numbers[] = {{4}, {7}};
static_assert(numbers[0].i == 4 && numbers[1].i == 7);

template <class Rep>
struct Duration {
  Rep count;
  template <class Rep2>
  constexpr explicit Duration(const Rep2& value)
      : count(static_cast<Rep>(value)) {}
};

inline constexpr Duration<long long> tai_offset{37000000};
inline constexpr Duration<double> half{0.5};
static_assert(tai_offset.count == 37000000);
static_assert(half.count == 0.5);

int main() {
  return encodings[1].name[0] == 'U' && outers[1].z == 3 ? 0 : 1;
}
