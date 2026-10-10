// RUN: -std=c++20
// EXPECT_EXIT: 0

// A block-scope alias of a dependent member type names the member chosen by
// the instantiated arguments, including a partial specialization's member.
// In a member template the alias mixes the class's and the member's own
// parameters, as std::shared_ptr's constructor does when picking a deleter.

#include <memory>

struct A {
  static constexpr int k = 1;
};
struct B {
  static constexpr int k = 2;
};

template <class T>
struct Pick {
  using type = A;
};
template <class T>
struct Pick<T*> {
  using type = B;
};

template <class T, class U>
struct Pick2 {
  using type = A;
};
template <class T, class U>
struct Pick2<T*, U> {
  using type = B;
};

template <class D>
int Get(D) {
  return D::k;
}

template <class T>
int UsingAlias() {
  using D = typename Pick<T>::type;
  return D::k;
}

template <class T>
int TypedefAlias() {
  typedef typename Pick<T>::type D;
  return D::k;
}

template <class T>
int Deduced(T) {
  using D = typename Pick<T>::type;
  return D::k;
}

template <class T>
struct S {
  int k = 0;
  S() = default;
  template <class U>
  explicit S(U*) {
    using D = typename Pick2<T, U>::type;
    k = Get(D());
  }
  template <class U>
  int Member(U*) {
    using D = typename Pick2<T, U>::type;
    return Get(D());
  }
  template <class U>
  static int Static(U*) {
    using D = typename Pick2<T, U>::type;
    return Get(D());
  }
  int Plain() {
    using D = typename Pick2<T, int>::type;
    return Get(D());
  }
};

int main() {
  if (UsingAlias<int*>() != 2 || UsingAlias<int>() != 1) return 1;
  if (TypedefAlias<char*>() != 2 || TypedefAlias<char>() != 1) return 2;
  int* p = nullptr;
  if (Deduced(p) != 2 || Deduced(0) != 1) return 3;

  int a = 0;
  if (S<int*>(&a).k != 2 || S<int>(&a).k != 1) return 4;
  S<int*> s;
  if (s.Member(&a) != 2) return 5;
  if (S<int*>::Static(&a) != 2 || S<int>::Static(&a) != 1) return 6;
  if (s.Plain() != 2 || S<int>().Plain() != 1) return 7;

  std::shared_ptr<int[]> array(new int[3]);
  if (std::get_deleter<std::default_delete<int>>(array) != nullptr) return 8;
  return 0;
}
