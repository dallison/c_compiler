// `decltype(f<U>())` inside a member function template of a class template
// names the member template's own parameter `U`.  Instantiating the class
// binds only the class parameters; `U` must stay dependent until the member
// template itself is instantiated, not be read as the class argument.
template <class T, T v>
struct Constant {
  static constexpr T value = v;
};

template <class A, class B>
struct Same {
  static constexpr bool value = false;
};
template <class A>
struct Same<A, A> {
  static constexpr bool value = true;
};

template <class A>
struct Box {
  static constexpr int value = sizeof(A);
};

template <class A>
constexpr auto DeducedSize() { return Constant<int, sizeof(A)>(); }

template <class A>
constexpr Constant<int, sizeof(A)> DeclaredSize() { return {}; }

template <class A>
constexpr Box<A> BoxOf() { return {}; }

template <class A>
constexpr auto IsLong() { return Constant<bool, Same<A, long long>::value>(); }

template <class P>
struct Traits {
  template <class U>
  static constexpr int Deduced() { return decltype(DeducedSize<U>())::value; }
  template <class U>
  static constexpr int Declared() {
    return decltype(DeclaredSize<U>())::value;
  }
  template <class U>
  static constexpr int Boxed() { return decltype(BoxOf<U>())::value; }
  template <class U>
  static auto Make() { return IsLong<U>(); }
  template <class U>
  static constexpr bool ViaMember() { return decltype(Make<U>())::value; }
};

int main() {
  if (Traits<char>::Deduced<long long>() != 8) return 1;
  if (Traits<short>::Deduced<char>() != 1) return 2;
  if (Traits<char>::Declared<long long>() != 8) return 3;
  if (Traits<short>::Declared<char>() != 1) return 4;
  if (Traits<char>::Boxed<int>() != 4) return 5;
  if (!Traits<int>::ViaMember<long long>()) return 6;
  if (Traits<long long>::ViaMember<int>()) return 7;
  return 0;
}
