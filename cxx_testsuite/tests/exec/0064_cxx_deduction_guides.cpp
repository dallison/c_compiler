namespace std {
template <class T>
struct initializer_list {
  const T* __begin;
  unsigned long __size;

  const T* begin() const {
    return __begin;
  }

  const T* end() const {
    return __begin + __size;
  }

  unsigned long size() const {
    return __size;
  }
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
  Holder(T initial) : value(initial + 1) {
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
  LongTag(int initial) : value(initial + 2) {
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
  int tag;
  ListChoice(std::initializer_list<T> values) {
    tag = 100 + (int)values.size();
  }
  ListChoice(int first, T second) {
    tag = 10 + first + second;
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
  ExactLongTag(int initial) : value(initial + 100) {
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
  const int const_value = 12;
  int first_pointer_value = 30;
  int second_pointer_value = 31;
  int first_array[2] = {32, 33};
  int second_array[2] = {34, 35};
  Pair aggregate{3, 4};
  IntPair partial_alias_pair{5, 'q'};
  SamePair same_alias_pair{6, 7};
  PointerPair pointer_alias_pair{&first_pointer_value, &second_pointer_value};
  ArrayPointerPair array_pointer_alias_pair{&first_array, &second_array};
  TransformIntFirst transformed_alias_choice('x');
  Pair copied = aggregate;
  Holder constructed(5);
  AliasHolder alias_constructed(6);
  auto functional_constructed = Holder(10);
  auto alias_functional_constructed = AliasHolder(11);
  int functional_argument = take_holder(Holder(12));
  auto heap_paren = new Holder(8);
  auto heap_brace = new Holder{10};
  Wrapped guided{6};
  Chosen selected{7};
  ExplicitOnly explicit_direct(8);
  ExplicitOnly explicit_list{9};
  ListChoice list_choice{1, 2};
  ListChoice copy_list_choice = {1, 2};
  ListChoice paren_choice(1, 2);
  RefChoice ref_choice(const_value);
  ref_choice.value = 13;
  PointerChoice pointer_choice("abc");
  ConstPointerChoice const_pointer_choice("def");
  RankChoice exact_choice(14);
  exact_choice.value = 14;
  DefaultAggregate defaulted{15};
  DefaultAggregate full_defaulted{16, 17};
  DerivedAggregate derived_base{18};
  DerivedAggregate full_derived_base{19, 20};
  ArrayAggregate array_aggregate{{21, 22, 23}};
  ArrayAggregate copied_array_aggregate = {{24, 25}};
  StringAggregate string_aggregate{"abc"};
  if (aggregate.first != 3 || aggregate.second != 4) {
    return 1;
  }
  if (partial_alias_pair.first != 5 || partial_alias_pair.second != 'q') {
    return 2;
  }
  if (same_alias_pair.first != 6 || same_alias_pair.second != 7) {
    return 3;
  }
  if (*pointer_alias_pair.first != 30 || *pointer_alias_pair.second != 31) {
    return 4;
  }
  if ((*array_pointer_alias_pair.first)[1] != 33 ||
      (*array_pointer_alias_pair.second)[1] != 35) {
    return 5;
  }
  if (sizeof(transformed_alias_choice.first) != sizeof(int) ||
      sizeof(transformed_alias_choice.second) != sizeof(long)) {
    return 6;
  }
  if (functional_constructed.value != 11 ||
      alias_functional_constructed.value != 12 ||
      functional_argument != 13) {
    return 7;
  }
  if (copied.first != 3 || copied.second != 4) {
    return 8;
  }
  if (constructed.value != 6) {
    return 9;
  }
  if (alias_constructed.value != 7) {
    return 10;
  }
  if (heap_paren->value != 9) {
    return 11;
  }
  if (heap_brace->value != 11) {
    return 12;
  }
  delete heap_paren;
  delete heap_brace;
  if (guided.value != 6) {
    return 13;
  }
  if (selected.value.value != 7) {
    return 14;
  }
  long explicit_direct_value = explicit_direct.value.value;
  long explicit_list_value = explicit_list.value.value;
  (void)explicit_direct_value;
  (void)explicit_list_value;
  if (list_choice.tag != 102) {
    return 15;
  }
  if (copy_list_choice.tag != 102) {
    return 16;
  }
  if (paren_choice.tag != 13) {
    return 17;
  }
  if (ref_choice.value != 13) {
    return 18;
  }
  if (pointer_choice.value != 'a') {
    return 19;
  }
  if (const_pointer_choice.value != 'd') {
    return 20;
  }
  if (exact_choice.value != 14) {
    return 21;
  }
  if (defaulted.first != 15 || defaulted.second != 42) {
    return 22;
  }
  if (full_defaulted.first != 16 || full_defaulted.second != 17) {
    return 23;
  }
  if (derived_base.base != 18 || derived_base.extra != 3) {
    return 24;
  }
  if (full_derived_base.base != 19 || full_derived_base.extra != 20) {
    return 25;
  }
  if (array_aggregate.values[0] != 21 ||
      array_aggregate.values[2] != 23) {
    return 26;
  }
  if (copied_array_aggregate.values[0] != 24 ||
      copied_array_aggregate.values[1] != 25) {
    return 27;
  }
  if (string_aggregate.text[0] != 'a' ||
      string_aggregate.text[3] != '\0') {
    return 28;
  }
  return 0;
}
