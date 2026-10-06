// RUN: -std=c++20 -fconstexpr-eval=audit

// Null and non-null pointers in constant expressions: const pointer objects
// initialized from 0, value-initialized scalars, pointers converted to bool,
// and null pointer members of objects built by constexpr constructors.

int* const zero_init = 0;
const int* const zero_long = 0L;

constexpr const int* null_p = nullptr;
static_assert(null_p == nullptr);
static_assert(!null_p);
static_assert(!(bool)null_p);
static_assert(null_p ? false : true);
static_assert(!(null_p && true));
static_assert(null_p || true);
constexpr bool null_p_is_null = null_p == nullptr;
static_assert(null_p_is_null);
constexpr double picked = null_p ? 1.0 : 2.0;
static_assert(picked == 2.0);

constexpr int i{};
static_assert(i == 0);
constexpr double d{};
static_assert(d == 0);
constexpr const int* braced_p{};
static_assert(braced_p == nullptr);
constexpr const int* equals_braced_p = {};
static_assert(!equals_braced_p);

constexpr int one = 1;
constexpr const int* one_p = &one;
static_assert(one_p);
static_assert(!!one_p);
static_assert(one_p != nullptr);

int global;
constexpr int* global_p = &global;
static_assert(global_p);
static_assert(!!global_p);

constexpr const char* text = "x";
static_assert(text);

struct InClass {
  static_assert(one_p);
};

template <class T>
struct InTemplate {
  static_assert(one_p && sizeof(T));
  static_assert(!(T*)nullptr);
};
InTemplate<int> in_template;

constexpr bool is_null(const int* p) { return !p; }
static_assert(is_null(nullptr));
static_assert(!is_null(&one));

constexpr int read_plus_one(const int* p) { return *p + 1; }
static_assert(read_plus_one(&one) == 2);
constexpr double two_and_a_half = 2.5;
constexpr double read_double(const double* p) { return *p; }
static_assert(read_double(&two_and_a_half) == 2.5);

constexpr bool conditions(const int* p) {
  if (p) {
    return false;
  }
  while (p) {
  }
  int x = 0;
  int* q = &x;
  return q && !p;
}
static_assert(conditions(nullptr));

void in_function() {
  constexpr int* local_p = nullptr;
  static_assert(local_p == nullptr);
  static_assert(one_p);
}

struct Q {
  const int* p;
  int k;
  constexpr Q() : p(nullptr), k(2) {}
};
constexpr Q q1{};
static_assert(q1.p == nullptr);
static_assert(!q1.p);
static_assert(q1.k == 2);
constexpr Q q2 = Q();
static_assert(q2.p == nullptr);
static_assert(q2.k == 2);
constexpr Q q3;
static_assert(q3.p == nullptr);
constexpr bool member_is_null() { return q1.p == nullptr; }
static_assert(member_is_null());

struct WithDefault {
  const int* p = nullptr;
  int k = 2;
};
constexpr WithDefault with_default{};
static_assert(!with_default.p);
static_assert(with_default.p == nullptr);
