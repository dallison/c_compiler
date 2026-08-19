// RUN: -std=c++29
// EXPECT_EXIT: 0

struct A {
  int a1;
  int a2;
};

struct B : A {
  int b;
};

struct F {
  int f;
};

struct G {
  int g;
};

struct H : F, G {
  int h;
};

struct C : B {
  int c;
};

struct Defaults {
  int first = 31;
  int second = 32;
};

struct WithDefaults : Defaults {
  int own;
};

template <typename T>
struct TemplateBase {
  T base;
};

template <typename T>
struct TemplateDerived : TemplateBase<T> {
  T own;
};

template <typename T>
TemplateDerived<T> make_template_derived(T base, T own) {
  return {.base = base, .own = own};
}

int main() {
  B flat{.a1 = 1, .a2 = 2, .b = 3};
  B braced{{.a1 = 4, .a2 = 5}, .b = 6};
  B typed{A{7, 8}, .b = 9};
  B direct{.a1{10}, .a2{11}, .b{12}};
  C recursive{.a1 = 13, .b = 14, .c = 15};
  H mixed{{.f = 16}, .g = 17, .h = 18};
  WithDefaults defaulted{.second = 33, .own = 34};
  auto templated = make_template_derived(40, 41);

  if (flat.a1 != 1 || flat.a2 != 2 || flat.b != 3 ||
      braced.a1 != 4 || braced.a2 != 5 || braced.b != 6 ||
      typed.a1 != 7 || typed.a2 != 8 || typed.b != 9 ||
      direct.a1 != 10 || direct.a2 != 11 || direct.b != 12 ||
      recursive.a1 != 13 || recursive.a2 != 0 || recursive.b != 14 ||
      recursive.c != 15 || mixed.f != 16 || mixed.g != 17 ||
      mixed.h != 18 || defaulted.first != 31 || defaulted.second != 33 ||
      defaulted.own != 34 || templated.base != 40 || templated.own != 41) {
    return 1;
  }
  return 0;
}
