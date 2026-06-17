// RUN: -std=c++20

int template_destructor_trace;

template <typename T>
struct Holder {
  T value;
};

template <typename T = int>
struct DefaultHolder {
  T value;
};

template <typename T = int, typename U = Holder<T> >
struct DefaultedPair {
  T first;
  U second;
};

template <typename T>
struct ConstructedHolder {
  T value;
  ConstructedHolder() {
    this->value = 0;
  }
  ConstructedHolder(T initial) {
    this->value = initial;
  }
};

template <typename T, typename U>
struct Pair {
  T first;
  U second;
};

template <int N = 4>
struct Buffer {
  int data[N];
};

template <int N, int M = N>
struct Matrix {
  int data[M];
};

template <typename T>
struct SpecializedHolder {
  T value;
};

template <>
struct SpecializedHolder<int> {
  int value;
  int bonus;
  static int specialized_static_value;
  int get_bonus(void);
  static int specialized_static_total(void);
  int inline_total(void) {
    return this->value + this->bonus + 1;
  }
};

int SpecializedHolder<int>::specialized_static_value = 31;

int SpecializedHolder<int>::get_bonus(void) {
  return this->bonus;
}

int SpecializedHolder<int>::specialized_static_total(void) {
  return SpecializedHolder<int>::specialized_static_value + 1;
}

template <typename T>
struct DeclaredSpecializedHolder {
  T value;
};

template <>
struct DeclaredSpecializedHolder<int>;

template <>
struct DeclaredSpecializedHolder<int> {
  int value;
  int bonus;
  int sum(void);
};

int DeclaredSpecializedHolder<int>::sum(void) {
  return this->value + this->bonus;
}

template <typename T = int>
struct DefaultSpecializedHolder {
  T value;
};

template <>
struct DefaultSpecializedHolder<> {
  int value;
  int bonus;
  static int default_static_value;
  int sum(void);
  static int default_static_total(void);
  int inline_total(void) {
    return this->value + this->bonus + 3;
  }
};

int DefaultSpecializedHolder<>::default_static_value = 33;

int DefaultSpecializedHolder<>::sum(void) {
  return this->value + this->bonus;
}

int DefaultSpecializedHolder<>::default_static_total(void) {
  return DefaultSpecializedHolder<>::default_static_value + 2;
}

template <typename T>
struct SpecializedLifecycleHolder {
  T value;
};

template <>
struct SpecializedLifecycleHolder<int> {
  int value;
  SpecializedLifecycleHolder(int initial);
  ~SpecializedLifecycleHolder();
};

SpecializedLifecycleHolder<int>::SpecializedLifecycleHolder(int initial) {
  this->value = initial + 1;
}

SpecializedLifecycleHolder<int>::~SpecializedLifecycleHolder() {
  template_destructor_trace = this->value + 2;
}

template <typename T>
struct WithMemberFunction {
  T value;
  T get(void);
  void set(T next);
  T add(T other);
  T inline_add(T other) {
    return this->value + other;
  }
  Holder<T> wrap(T next);
  T unwrap(Holder<T> holder);
  Holder<T> inline_wrap(T next) {
    Holder<T> holder;
    holder.value = next;
    return holder;
  }
};

template <typename T>
struct WithMemberTemplate {
  T value;

  template <typename U>
  U choose(T input, U fallback) {
    this->value = input;
    return fallback;
  }

  template <typename U>
  U explicit_choose(T input, U fallback) {
    this->value = input;
    return fallback + 1;
  }

  template <typename U>
  U out_of_class_choose(T input, U fallback);
};

template <typename T>
template <typename U>
U WithMemberTemplate<T>::out_of_class_choose(T input, U fallback) {
  this->value = input;
  return fallback + 2;
}

template <typename T, int N>
struct WithDependentMember {
  ConstructedHolder<T> stored;
  T marker;
  Buffer<N> buffer;
  WithDependentMember() : stored(), marker(0) {
    this->buffer.data[0] = 0;
  }
  WithDependentMember(T initial) : stored(initial), marker(initial) {
    this->buffer.data[0] = initial;
  }
};

template <typename T>
struct WithOutOfClassCtor {
  ConstructedHolder<T> stored;
  T marker;
  WithOutOfClassCtor(T initial);
};

template <typename T>
WithOutOfClassCtor<T>::WithOutOfClassCtor(T initial)
    : stored(initial), marker(initial) {
}

template <typename T>
struct WithOutOfClassDtor {
  T value;
  WithOutOfClassDtor(T initial) {
    this->value = initial;
  }
  ~WithOutOfClassDtor();
};

template <typename T>
WithOutOfClassDtor<T>::~WithOutOfClassDtor() {
  template_destructor_trace = this->value;
}

template struct WithOutOfClassCtor<long>;

template <typename T>
T WithMemberFunction<T>::get(void) {
  return this->value;
}

template <typename T>
void WithMemberFunction<T>::set(T next) {
  this->value = next;
}

template <typename T>
T WithMemberFunction<T>::add(T other) {
  return this->value + other;
}

template <typename T>
Holder<T> WithMemberFunction<T>::wrap(T next) {
  Holder<T> holder;
  holder.value = next;
  return holder;
}

template <typename T>
T WithMemberFunction<T>::unwrap(Holder<T> holder) {
  return holder.value;
}

namespace cache {
template <typename T>
struct Box {
  T value;
};
}

template struct cache::Box<int>;

template <>
struct cache::Box<long>;

template <>
struct cache::Box<long> {
  long value;
  long bonus;
  static long box_static_value;
  long sum(void);
  static long box_static_total(void);
  long inline_sum(void) {
    return this->value + this->bonus + 5;
  }
};

long cache::Box<long>::box_static_value = 37;

long cache::Box<long>::sum(void) {
  return this->value + this->bonus;
}

long cache::Box<long>::box_static_total(void) {
  return cache::Box<long>::box_static_value + 3;
}

namespace left {
struct Item {
  int left_value;
};
}

namespace right {
struct Item {
  int right_value;
};
}

using IntAlias = int;

struct InlinePlain {
  int value;
  int bump(int next) {
    this->value = this->value + next;
    return this->value;
  }
};

Holder<int> make_holder(void);
Buffer<4> make_buffer4(void);
cache::Box<int> make_box(void);

int main(void) {
  Holder<int> local;
  Holder<int> again;
  local.value = 7;
  again.value = local.value;
  Pair<int, char> pair;
  pair.first = again.value;
  pair.second = 3;
  Buffer<4> buffer;
  Buffer<4> same_buffer;
  Buffer<2 + 2> expression_buffer;
  Buffer<8> bigger_buffer;
  Buffer<> default_buffer;
  Buffer<2> explicit_default_buffer;
  Matrix<4> default_matrix;
  Matrix<2, 3> explicit_matrix;
  SpecializedHolder<int> specialized_holder;
  SpecializedHolder<char> primary_holder;
  DeclaredSpecializedHolder<int> declared_specialized_holder;
  DeclaredSpecializedHolder<char> declared_primary_holder;
  DefaultSpecializedHolder<> default_specialized_holder;
  DefaultSpecializedHolder<char> default_primary_holder;
  int specialized_lifecycle_trace;
  WithMemberFunction<int> member_function_holder;
  WithMemberFunction<char> char_member_function_holder;
  WithMemberTemplate<int> member_template_holder;
  WithDependentMember<int, 4> dependent_member_holder;
  WithDependentMember<char, 2 + 2> dependent_char_member_holder;
  WithDependentMember<int, 4> constructed_member(9);
  WithOutOfClassCtor<int> out_of_class_constructed(11);
  enum { enum_count = 4 };
  Buffer<enum_count> enum_buffer;
  const int const_count = 4;
  Buffer<const_count> const_buffer;
  buffer.data[3] = pair.second;
  same_buffer.data[0] = buffer.data[3];
  expression_buffer.data[3] = same_buffer.data[0];
  enum_buffer.data[3] = expression_buffer.data[3];
  const_buffer.data[3] = enum_buffer.data[3];
  bigger_buffer.data[7] = const_buffer.data[3];
  default_buffer.data[3] = 11;
  explicit_default_buffer.data[1] = 12;
  default_matrix.data[3] = 13;
  explicit_matrix.data[2] = 14;
  specialized_holder.value = 15;
  specialized_holder.bonus = 16;
  primary_holder.value = 17;
  declared_specialized_holder.value = 18;
  declared_specialized_holder.bonus = 19;
  declared_primary_holder.value = 20;
  default_specialized_holder.value = 21;
  default_specialized_holder.bonus = 22;
  default_primary_holder.value = 23;
  {
    SpecializedLifecycleHolder<int> lifecycle_holder(24);
    specialized_lifecycle_trace = lifecycle_holder.value;
  }
  specialized_lifecycle_trace =
      specialized_lifecycle_trace + template_destructor_trace;
  cache::Box<int> box;
  box.value = bigger_buffer.data[7];
  cache::Box<long> specialized_box;
  specialized_box.value = box.value;
  specialized_box.bonus = 24;
  member_function_holder.value = box.value;
  member_function_holder.set(member_function_holder.get());
  member_function_holder.value = member_function_holder.add(1);
  Holder<int> wrapped_member = member_function_holder.wrap(3);
  member_function_holder.value = member_function_holder.unwrap(wrapped_member);
  Holder<int> inline_wrapped_member = member_function_holder.inline_wrap(4);
  member_function_holder.value =
      member_function_holder.inline_add(inline_wrapped_member.value);
  char_member_function_holder.set(5);
  char_member_function_holder.value = char_member_function_holder.add(1);
  Holder<char> wrapped_char_member = char_member_function_holder.wrap(2);
  char_member_function_holder.value =
      char_member_function_holder.unwrap(wrapped_char_member);
  char deduced_member_template = member_template_holder.choose(9, 'a');
  char explicit_member_template =
      member_template_holder.explicit_choose<char>(10, 'b');
  char out_of_class_member_template =
      member_template_holder.out_of_class_choose<char>(11, 'c');
  dependent_member_holder.stored.value = member_function_holder.value;
  dependent_member_holder.buffer.data[3] =
      dependent_member_holder.stored.value;
  dependent_char_member_holder.stored.value = char_member_function_holder.value;
  dependent_char_member_holder.buffer.data[3] =
      dependent_char_member_holder.stored.value;
  constructed_member.buffer.data[3] = constructed_member.buffer.data[0];
  out_of_class_constructed.marker =
      out_of_class_constructed.stored.value + out_of_class_constructed.marker;
  {
    WithOutOfClassDtor<int> destroyed_at_block_exit(13);
    template_destructor_trace = destroyed_at_block_exit.value - 13;
  }
  Holder<IntAlias> alias_holder;
  Holder<const int> const_holder;
  Holder<int*> pointer_holder;
  Holder<int&> reference_holder;
  DefaultHolder<> default_holder;
  DefaultHolder<char> explicit_default_holder;
  DefaultedPair<> default_pair;
  DefaultedPair<char> partial_default_pair;
  cache::Box<left::Item> left_box;
  cache::Box<right::Item> right_box;
  left_box.value.left_value = 1;
  right_box.value.right_value = 2;
  default_holder.value = 5;
  explicit_default_holder.value = 6;
  default_pair.first = 7;
  default_pair.second.value = 8;
  partial_default_pair.first = 9;
  partial_default_pair.second.value = 10;
  InlinePlain plain;
  plain.value = 4;
  int plain_result = plain.bump(3);
  return pair.first + box.value + deduced_member_template +
         explicit_member_template + out_of_class_member_template +
         default_holder.value + explicit_default_holder.value +
         default_pair.first + default_pair.second.value +
         partial_default_pair.first + partial_default_pair.second.value +
         default_buffer.data[3] + explicit_default_buffer.data[1] +
         default_matrix.data[3] + explicit_matrix.data[2] +
         specialized_holder.value + specialized_holder.bonus +
         specialized_holder.get_bonus() +
         specialized_holder.inline_total() +
         SpecializedHolder<int>::specialized_static_value +
         SpecializedHolder<int>::specialized_static_total() +
         primary_holder.value + declared_specialized_holder.value +
         declared_specialized_holder.bonus +
         declared_specialized_holder.sum() + declared_primary_holder.value +
         default_specialized_holder.value + default_specialized_holder.bonus +
         default_specialized_holder.sum() +
         default_specialized_holder.inline_total() +
         DefaultSpecializedHolder<>::default_static_value +
         DefaultSpecializedHolder<>::default_static_total() +
         default_primary_holder.value + specialized_lifecycle_trace +
         specialized_box.bonus +
         specialized_box.sum() +
         specialized_box.inline_sum() +
         cache::Box<long>::box_static_value +
         cache::Box<long>::box_static_total() +
         left_box.value.left_value + right_box.value.right_value +
         plain_result;
}
