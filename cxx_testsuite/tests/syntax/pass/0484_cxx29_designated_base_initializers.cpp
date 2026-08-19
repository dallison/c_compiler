// RUN: -std=c++29

#include <version>

#if __cpp_designated_initializers != 202606L
#error "P2287R6 feature-test macro has the wrong value"
#endif

struct A {
  int a1;
  int a2;
};

struct B : A {
  int b;
};

constexpr B flat{.a1 = 1, .a2 = 2, .b = 3};
constexpr B braced{{.a1 = 4, .a2 = 5}, .b = 6};
constexpr B typed{A{7, 8}, .b = 9};
constexpr B direct{.a1{10}, .a2{11}, .b{12}};

static_assert(flat.a1 == 1 && flat.a2 == 2 && flat.b == 3);
static_assert(braced.a1 == 4 && braced.a2 == 5 && braced.b == 6);
static_assert(typed.a1 == 7 && typed.a2 == 8 && typed.b == 9);
static_assert(direct.a1 == 10 && direct.a2 == 11 && direct.b == 12);

struct C : B {
  int c;
};

constexpr C recursive{.a1 = 13, .b = 14, .c = 15};
static_assert(recursive.a1 == 13 && recursive.a2 == 0 &&
              recursive.b == 14 && recursive.c == 15);

struct Defaults {
  int first = 31;
  int second = 32;
};

struct WithDefaults : Defaults {
  int own;
};

constexpr WithDefaults defaulted{.second = 33, .own = 34};
static_assert(defaulted.first == 31 && defaulted.second == 33 &&
              defaulted.own == 34);

struct F {
  int f;
};

struct G {
  int g;
};

struct H : F, G {
  int h;
};

constexpr H mixed_bases{{.f = 16}, .g = 17, .h = 18};
static_assert(mixed_bases.f == 16);
static_assert(mixed_bases.g == 17);
static_assert(mixed_bases.h == 18);

struct Shadow : A {
  int a1;
};

Shadow shadowed{.a1 = 19};

template <typename T>
struct TemplateBase {
  T base;
};

template <typename T>
struct TemplateDerived : TemplateBase<T> {
  T own;
};

constexpr TemplateDerived<int> template_value{.base = 40, .own = 41};
static_assert(template_value.base == 40 && template_value.own == 41);

int main() { return 0; }
