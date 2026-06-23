// RUN: -std=c++20

namespace std {
template <class T>
struct initializer_list {
  const T* __begin;
  unsigned long __size;
};
}

template <typename T, typename U>
struct Pair {
  T first;
  U second;
};

template <typename U>
using IntPair = Pair<int, U>;

template <typename T>
using SamePair = Pair<T, T>;

template <typename T>
using PointerPair = Pair<T*, T*>;

template <typename T, int N>
using ArrayPointerPair = Pair<T (*)[N], T (*)[N]>;

template <typename T, typename U>
struct TransformChoice {
  T first;
  U second;
  TransformChoice(char value) : first(value), second(0) {
  }
  TransformChoice(long value) : first(0), second(value) {
  }
};

TransformChoice(char) -> TransformChoice<char, int>;
TransformChoice(long) -> TransformChoice<int, long>;

template <typename U>
using TransformIntFirst = TransformChoice<int, U>;

template <typename T>
struct Holder {
  T value;
  Holder(T initial) : value(initial) {
  }
};

template <typename T>
using AliasHolder = Holder<T>;

template <typename T>
struct Wrapped {
  T value;
};

template <typename T>
Wrapped(T) -> Wrapped<T>;

struct LongTag {
  long value;
  LongTag(int initial) : value(initial) {
  }
};

template <typename T>
struct Chosen {
  T value;
};

Chosen(int) -> Chosen<LongTag>;

template <typename T>
struct ExplicitOnly {
  T value;
};

explicit ExplicitOnly(int) -> ExplicitOnly<LongTag>;

template <typename T>
struct ListChoice {
  ListChoice(std::initializer_list<T> values) {
  }
  ListChoice(int first, T second) {
  }
};

template <typename T>
struct RefChoice {
  T value;
  RefChoice(const T& initial) : value(initial) {
  }
};

template <typename T>
struct PointerChoice {
  T value;
  PointerChoice(T* ptr) : value(*ptr) {
  }
};

template <typename T>
struct ConstPointerChoice {
  T value;
  ConstPointerChoice(const T* ptr) : value(*ptr) {
  }
};

struct ExactLongTag {
  long value;
  ExactLongTag(int initial) : value(initial) {
  }
};

template <typename T>
struct RankChoice {
  T value;
};

RankChoice(int) -> RankChoice<int>;
RankChoice(long) -> RankChoice<ExactLongTag>;

template <typename T>
struct DefaultAggregate {
  T first;
  int second = 42;
};

template <typename T>
struct AggregateBase {
  T base;
};

template <typename T>
struct DerivedAggregate : AggregateBase<T> {
  int extra = 3;
};

template <typename T, int N>
struct ArrayAggregate {
  T values[N];
};

template <int N>
struct StringAggregate {
  char text[N];
};

int take_holder(Holder<int> value) {
  return value.value;
}

int main(void) {
  const int const_value = 4;
  int first_pointer_value = 5;
  int second_pointer_value = 6;
  int first_array[2] = {7, 8};
  int second_array[2] = {9, 10};
  Pair aggregate{1, 'a'};
  IntPair partial_alias_pair{2, 'b'};
  SamePair same_alias_pair{3, 4};
  PointerPair pointer_alias_pair{&first_pointer_value, &second_pointer_value};
  ArrayPointerPair array_pointer_alias_pair{&first_array, &second_array};
  TransformIntFirst transformed_alias_choice('x');
  Pair copied = aggregate;
  Holder constructed(7);
  AliasHolder alias_constructed(6);
  auto functional_constructed = Holder(10);
  auto alias_functional_constructed = AliasHolder(11);
  int functional_argument = take_holder(Holder(12));
  auto heap_paren = new Holder(8);
  auto heap_brace = new Holder{9};
  Wrapped guided(3);
  Chosen non_template{1};
  ExplicitOnly explicit_direct(2);
  ExplicitOnly explicit_list{3};
  ListChoice list_choice{1, 2};
  ListChoice copy_list_choice = {1, 2};
  ListChoice paren_choice(1, 2);
  RefChoice ref_choice(const_value);
  ref_choice.value = 5;
  PointerChoice pointer_choice("abc");
  ConstPointerChoice const_pointer_choice("def");
  RankChoice exact_choice(6);
  DefaultAggregate defaulted{7};
  DefaultAggregate full_defaulted{8, 9};
  DerivedAggregate derived_base{10};
  DerivedAggregate full_derived_base{11, 12};
  ArrayAggregate array_aggregate{{13, 14, 15}};
  ArrayAggregate copied_array_aggregate = {{16, 17}};
  StringAggregate string_aggregate{"abc"};
  (void)aggregate;
  (void)partial_alias_pair;
  (void)same_alias_pair;
  (void)pointer_alias_pair;
  (void)array_pointer_alias_pair;
  (void)transformed_alias_choice;
  (void)copied;
  (void)constructed;
  (void)alias_constructed;
  (void)functional_constructed;
  (void)alias_functional_constructed;
  (void)functional_argument;
  delete heap_paren;
  delete heap_brace;
  (void)guided;
  (void)non_template;
  (void)explicit_direct;
  (void)explicit_list;
  (void)list_choice;
  (void)copy_list_choice;
  (void)paren_choice;
  (void)ref_choice;
  (void)pointer_choice;
  (void)const_pointer_choice;
  (void)exact_choice;
  (void)defaulted;
  (void)full_defaulted;
  (void)derived_base;
  (void)full_derived_base;
  (void)array_aggregate;
  (void)copied_array_aggregate;
  (void)string_aggregate;
  return 0;
}
