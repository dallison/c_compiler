// RUN: -std=c++20

using size_type = unsigned long;

enum EvaluatorEnum {
  enum_base = 7,
  enum_computed = enum_base * 3 - 1,
  enum_conditional = 0 ? (1 / 0) : enum_computed,
};

const int const_int = enum_conditional + sizeof(int);
const double folded_double = (1.5 + 2.25) * 2.0;

static_assert('A' == 65, "char constants fold");
static_assert(L'A' == 65, "wide char constants fold");
static_assert(+5 == 5, "unary plus folds");
static_assert(-5 < 0, "unary minus folds");
static_assert(~0 == -1, "ones complement folds");
static_assert(((5 + 3 * 2 - 4 / 2) % 5) == 4, "arithmetic folds");
static_assert(((1 << 5) >> 2) == 8, "shifts fold");
static_assert((0x55 & 0x0f) == 5, "bitwise and folds");
static_assert((0x50 | 0x05) == 0x55, "bitwise or folds");
static_assert((0x55 ^ 0x0f) == 0x5a, "bitwise xor folds");

static_assert(static_cast<unsigned char>(-1) == 255,
              "unsigned char cast wraps");
static_assert(static_cast<signed char>(255) == -1,
              "signed char cast sign extends");
static_assert(static_cast<unsigned short>(-1) == 65535,
              "unsigned short cast wraps");
static_assert(static_cast<bool>(42), "nonzero converts to true");
static_assert(!static_cast<bool>(0), "zero converts to false");

static_assert(static_cast<size_type>(-1) / sizeof(int) ==
                  4611686018427387903UL,
              "unsigned division after cast folds");
static_assert(static_cast<size_type>(-1) % sizeof(int) == 3,
              "unsigned modulo after cast folds");
static_assert((-1 / 2U) == 2147483647U,
              "mixed signed unsigned division folds at unsigned width");
static_assert((-1 % 2U) == 1U,
              "mixed signed unsigned modulo folds at unsigned width");
static_assert(static_cast<size_type>(-1) > static_cast<size_type>(0),
              "unsigned comparison after cast folds");
static_assert(static_cast<unsigned>(-1) == 4294967295U,
              "unsigned equality after cast folds");
static_assert(static_cast<unsigned>(-1) == -1,
              "mixed signed unsigned equality folds after conversion");
static_assert(-1 < 0, "signed comparison stays signed");
static_assert(!(-1 > 0), "signed greater-than stays signed");

static_assert(1.5 != 1.0, "floating inequality folds");
static_assert(1.25 < 2.5, "floating less-than folds");
static_assert((1.5 + 0.25) == 1.75, "floating arithmetic folds");
static_assert(folded_double > 7.4 && folded_double < 7.6,
              "const double identifier folds");

static_assert(!(0 && (1 / 0)), "integer logical-and short-circuits false");
static_assert(1 || (1 / 0), "integer logical-or short-circuits true");
static_assert(!((1.0 == 2.0) && ((1.0 / 0.0) != 0.0)),
              "logical-and short-circuits a false floating comparison");
static_assert((1.0 == 1.0) || ((1.0 / 0.0) != 0.0),
              "logical-or short-circuits a true floating comparison");
static_assert((0 ? (1 / 0) : 9) == 9, "conditional folds false branch");
static_assert((1 ? 10 : (1 / 0)) == 10, "conditional folds true branch");

static_assert(sizeof(int) >= 2, "sizeof type folds");
static_assert(sizeof((int)0) == sizeof(int), "sizeof expression folds");
static_assert(enum_conditional == 20, "enum initializer chain folds");
static_assert(const_int == 20 + sizeof(int), "const identifier folds");

int array_bound[(enum_conditional == 20 && const_int > 20) ? 1 : -1];

int main(void) {
  switch (enum_conditional) {
    case 20:
      return array_bound[0];
    default:
      return 1;
  }
}
