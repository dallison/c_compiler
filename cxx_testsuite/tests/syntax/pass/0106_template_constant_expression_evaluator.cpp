// RUN: -std=c++20

template <int N>
struct IntValue {
};

template <unsigned long N>
struct ULongValue {
};

template <bool B>
struct BoolValue {
};

template <class A, class B>
struct Same {
  enum { value = 0 };
};

template <class A>
struct Same<A, A> {
  enum { value = 1 };
};

template <class T>
struct DependentLimit {
  using size_type = unsigned long;
  using max_type =
      ULongValue<static_cast<size_type>(-1) / sizeof(T)>;
  using room_type =
      BoolValue<(static_cast<size_type>(-1) / sizeof(T)) ==
                4611686018427387903UL>;
};

template <class T,
          unsigned long N = static_cast<unsigned long>(-1) / sizeof(T)>
struct DefaultLimit {
  using type = ULongValue<N>;
};

template <class T, int N = (0 ? (1 / 0) : sizeof(T) + 3)>
struct DefaultInt {
  using type = IntValue<N>;
};

template <class T, bool B = !(0 && (1 / 0))>
struct ShortCircuitDefault {
  using type = BoolValue<B>;
};

static_assert(Same<IntValue<(0 ? (1 / 0) : 12) >, IntValue<12> >::value,
              "conditional template argument folds selected arm");
static_assert(Same<IntValue<static_cast<int>(static_cast<unsigned char>(-1)) >,
                   IntValue<255> >::value,
              "cast template argument folds");
static_assert(Same<ULongValue<static_cast<unsigned long>(-1) / sizeof(int)>,
                   ULongValue<4611686018427387903UL> >::value,
              "unsigned static_cast template argument folds");
static_assert(Same<typename DependentLimit<int>::max_type,
                   ULongValue<4611686018427387903UL> >::value,
              "dependent member type argument folds after instantiation");
static_assert(Same<typename DependentLimit<int>::room_type,
                   BoolValue<true> >::value,
              "dependent unsigned comparison folds after instantiation");
static_assert(Same<typename DefaultLimit<int>::type,
                   ULongValue<4611686018427387903UL> >::value,
              "dependent default template argument folds");
static_assert(Same<typename DefaultInt<int>::type, IntValue<sizeof(int) + 3> >::
                  value,
              "dependent integral default folds");
static_assert(Same<typename ShortCircuitDefault<int>::type, BoolValue<true> >::
                  value,
              "dependent logical default short-circuits");

int main(void) {
  return 0;
}
