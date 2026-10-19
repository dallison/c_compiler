// RUN: -std=c++17
// EXPECT_EXIT: 0
// A member function template of a class template computes a type index with
// an overloaded constexpr function template whose explicit arguments name the
// member's own parameter (protobuf's FlatAllocation::BeginOffset).  While only
// the class arguments are bound, the call must stay dependent: neither the
// unary overload nor its result may be bound into the instantiated class.
template <int I> struct Pick;
template <> struct Pick<0> { using type = char; };
template <> struct Pick<1> { using type = int; };

template <int I> struct Scaled { static constexpr int value = I * 10; };

template <int I, class... T> struct At;
template <class H, class... T> struct At<0, H, T...> { using type = H; };
template <int I, class H, class... T> struct At<I, H, T...> {
  using type = typename At<I - 1, T...>::type;
};

template <class U> constexpr int Ov() { return 5; }
template <class U, class V, class W> constexpr int Ov() {
  return sizeof(U) == 1 ? 0 : 1;
}

template <class A, class B> struct Same { static constexpr bool value = false; };
template <class A> struct Same<A, A> { static constexpr bool value = true; };

template <typename T> constexpr int Find() { return -1; }
template <typename T, typename T1, typename... Ts> constexpr int Find() {
  return Same<T, T1>::value ? 0 : Find<T, Ts...>() + 1;
}

template <class K> int SizeOf() { return (int)sizeof(K); }

struct Map {
  template <class K> int Get() const { return (int)sizeof(K); }
};

template <typename... T> struct Alloc {
  Map ends;
  template <typename U> int Direct() const {
    return SizeOf<typename Pick<Ov<U, char, short>()>::type>();
  }
  template <typename U> int Local() const {
    constexpr int i = Ov<U, char, short>();
    return Scaled<i>::value;
  }
  template <typename U> int Member() const {
    constexpr int i = Ov<U, char, short>();
    using P = typename Pick<i>::type;
    return ends.template Get<P>();
  }
  template <typename U> int BeginOffset() const {
    constexpr int type_index = Find<U, T...>();
    constexpr int prev = type_index == 0 ? 0 : type_index - 1;
    using PrevType = typename At<prev, T...>::type;
    return type_index == 0 ? 100 : ends.template Get<PrevType>();
  }
};

int main() {
  Alloc<char, short, int> a;
  if (a.Direct<char>() != 1 || a.Direct<int>() != 4) return 1;
  if (a.Local<char>() != 0 || a.Local<int>() != 10) return 2;
  if (a.Member<char>() != 1 || a.Member<int>() != 4) return 3;
  if (a.BeginOffset<char>() != 100 || a.BeginOffset<int>() != 2) return 4;
  return 0;
}
