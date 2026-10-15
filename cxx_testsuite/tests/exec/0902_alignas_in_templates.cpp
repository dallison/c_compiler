// alignas in templates: several specifiers on one declaration (the strictest
// applies, dependent or not), non-dependent specifiers surviving
// instantiation, type and value pack expansions, nested classes and member
// templates of class templates, and locals of function templates.
#include <stddef.h>
struct alignas(64) B64 { char c; };
template <typename T> int one() { char pad; alignas(T) char b[3]; (void)pad; return (size_t)b % alignof(T) == 0; }
template <typename T> int two() { char pad; alignas(T) alignas(4) char b[3]; (void)pad; return (size_t)b % alignof(T) == 0; }
template <typename T> int three() { char pad; alignas(4) alignas(T) char b[3]; (void)pad; return (size_t)b % alignof(T) == 0; }
template <size_t N> int four() { char pad; alignas(N) char b[3]; (void)pad; return (size_t)b % N == 0; }
template <typename... Ts> int five() { char pad; alignas(Ts...) char b[3]; (void)pad; return (size_t)b % 64 == 0; }
static int LocalChecks() {
  int r = 0;
  if (!one<B64>()) r |= 1;
  if (!two<B64>()) r |= 2;
  if (!three<B64>()) r |= 4;
  if (!four<64>()) r |= 8;
  if (!five<char, B64>()) r |= 16;
  return r;
}


struct alignas(16) Big16 { char c; };

template <typename T>
struct Member {
  char c;
  alignas(32) alignas(T) char m;   // max(32, alignof(T))
};

template <typename T>
struct Member2 {
  char c;
  alignas(T) alignas(8) char m;    // max(alignof(T), 8)
};

template <typename T>
struct alignas(T) ClassT { char c; };

template <typename T>
struct alignas(T) alignas(16) ClassT2 { char c; };

template <size_t N>
struct alignas(N) ClassN { char c; };

template <typename... Ts>
struct alignas(Ts...) ClassPack { char c; };

template <typename... Ts>
struct PackMember {
  char c;
  alignas(Ts...) char m;
};

template <size_t... Ns>
struct alignas(Ns...) ClassNPack { char c; };

template <typename T>
struct Var {
  static int f() {
    alignas(T) alignas(4) char local[3];
    return (int)((size_t)&local % alignof(T) == 0);
  }
};

struct alignas(8) alignas(32) alignas(16) NonTemplate { char c; };
struct NonTemplateMember { char c; alignas(4) alignas(64) alignas(8) char m; };

template <typename... Ts>
struct EmptyPack { char c; alignas(Ts...) char m; };

template <typename T>
struct Outer {
  template <typename U>
  struct Inner { char c; alignas(T) alignas(U) alignas(2) char m; };
};

template <typename T>
struct NestOuter {
  struct alignas(T) alignas(2) Plain { char c; };
  struct alignas(T) WithFn { char c; int f() const { return c; } };
  template <typename U>
  struct alignas(T) alignas(U) NestedT { char c; };
  template <typename... Us>
  struct alignas(Us...) NestedPack { char c; };
  template <size_t... Ns>
  struct Mem { char c; alignas(T) alignas(Ns...) char m; };
};

template <typename T>
struct Gnu { char c; alignas(T) __attribute__((aligned(64))) char m; };

template <typename T>
struct Derived : Member<T> { alignas(T) char d; };

template <typename... Ts>
int local_pack() {
  alignas(Ts...) char buf[1];
  return (int)((size_t)buf % 32 == 0);
}

template <class T> struct PlainMember { char c; alignas(32) T m; };
template <class T> struct alignas(64) PlainClass { T c; };
static_assert(alignof(PlainMember<int>) == 32, "PlainMember<int>");
static_assert(offsetof(PlainMember<int>, m) == 32, "PlainMember<int>::m");
static_assert(alignof(PlainClass<int>) == 64, "PlainClass<int>");

using PM = PackMember<short, double>;

int main() {
  static_assert(alignof(Member<char>) == 32, "Member<char>");
  static_assert(alignof(Member<Big16>) == 32, "Member<Big16>");
  static_assert(offsetof(Member<char>, m) == 32, "Member<char>::m");
  static_assert(alignof(Member2<char>) == 8, "Member2<char>");
  static_assert(alignof(Member2<Big16>) == 16, "Member2<Big16>");
  static_assert(alignof(Member2<double>) == 8, "Member2<double>");
  static_assert(alignof(ClassT<double>) == 8, "ClassT<double>");
  static_assert(alignof(ClassT<Big16>) == 16, "ClassT<Big16>");
  static_assert(sizeof(ClassT<Big16>) == 16, "sizeof ClassT<Big16>");
  static_assert(alignof(ClassT2<char>) == 16, "ClassT2<char>");
  static_assert(alignof(ClassN<64>) == 64, "ClassN<64>");
  static_assert(alignof(ClassPack<char, Big16, int>) == 16, "ClassPack");
  static_assert(alignof(PackMember<short, double>) == 8, "PackMember");
  static_assert(offsetof(PM, m) == 8, "PackMember::m");
  static_assert(alignof(ClassNPack<4, 32, 8>) == 32, "ClassNPack");
  if (!Var<Big16>::f()) return 1;
  static_assert(alignof(NonTemplate) == 32, "NonTemplate");
  static_assert(alignof(NonTemplateMember) == 64, "NonTemplateMember");
  static_assert(alignof(EmptyPack<>) == 1, "EmptyPack<>");
  static_assert(sizeof(EmptyPack<>) == 2, "sizeof EmptyPack<>");
  static_assert(alignof(Outer<char>::Inner<Big16>) == 16, "Inner<Big16>");
  static_assert(alignof(Outer<double>::Inner<char>) == 8, "Inner<char>");
  static_assert(alignof(Outer<char>::Inner<char>) == 2, "Inner<char,char>");
  static_assert(alignof(Gnu<char>) == 64, "Gnu<char>");
  static_assert(alignof(Derived<char>) == 32, "Derived<char>");
  static_assert(alignof(NestOuter<Big16>::Plain) == 16, "Plain");
  static_assert(alignof(NestOuter<char>::Plain) == 2, "Plain<char>");
  static_assert(alignof(NestOuter<double>::WithFn) == 8, "WithFn");
  static_assert(alignof(NestOuter<char>::NestedT<Big16>) == 16, "NestedT");
  static_assert(alignof(NestOuter<Big16>::NestedT<char>) == 16, "NestedT2");
  static_assert(alignof(NestOuter<char>::NestedPack<int, Big16>) == 16, "NestedPack");
  static_assert(alignof(NestOuter<double>::Mem<2, 4>) == 8, "Mem<2,4>");
  static_assert(alignof(NestOuter<char>::Mem<2, 64>) == 64, "Mem<2,64>");
  if (!local_pack<char, Member<char>, int>()) return 2;
  if (int r = LocalChecks()) return 10 + r;
  return 0;
}
