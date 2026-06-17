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
struct NestedOwner {
  struct Inner {
    T value;
  };
};

template <typename T, typename U>
struct PairNestedOwner {
  struct Inner {
    T first;
    U second;
  };
};

template <typename T, int N>
struct SizedNestedOwner {
  struct Inner {
    T value;
    int data[N];
  };
};

template <typename T, int N>
struct SizedMemberOwner {
  struct Inner {
    T value;
    int data[N];
  };
  Inner stored;
};

template <typename T>
struct UsesNested {
  typename NestedOwner<T>::Inner value;
};

namespace nested_ns {
template <typename T>
struct Owner {
  struct Inner {
    T value;
  };
};

template <typename T, int N>
struct SizedOwner {
  struct Inner {
    T value;
    int data[N];
  };
};

template <typename T, int N>
struct SizedMemberOwner {
  struct Inner {
    T value;
    int data[N];
  };
  Inner stored;
};

}

template <typename T>
struct UsesNamespacedNested {
  typename nested_ns::Owner<T>::Inner value;
};

template <typename T>
struct UsesWrappedNested {
  Holder<typename NestedOwner<T>::Inner> value;
};

template <typename T, int N>
struct UsesSizedNested {
  typename SizedNestedOwner<T, N>::Inner value;
};

template <typename T, int N>
struct UsesWrappedSizedNested {
  Holder<typename SizedNestedOwner<T, N>::Inner> value;
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

NestedOwner<int>::Inner make_nested_inner(void) {
  NestedOwner<int>::Inner result;
  result.value = 19;
  return result;
}

int read_nested_inner(NestedOwner<int>::Inner value) {
  return value.value;
}

template <typename T>
typename NestedOwner<T>::Inner make_dependent_nested(T value) {
  typename NestedOwner<T>::Inner result;
  result.value = value;
  return result;
}

template <typename T>
T read_dependent_nested(typename NestedOwner<T>::Inner value) {
  return value.value;
}

template <typename T, typename U>
int read_pair_dependent_nested(
    typename PairNestedOwner<T, U>::Inner value) {
  return value.first + value.second;
}

template <typename T, int N>
int read_sized_dependent_nested(
    typename SizedNestedOwner<T, N>::Inner value) {
  return value.value + value.data[2];
}

template <typename T, int N>
typename SizedNestedOwner<T, N>::Inner make_sized_dependent_nested(
    T value, int extra) {
  typename SizedNestedOwner<T, N>::Inner result;
  result.value = value;
  result.data[2] = extra;
  return result;
}

template <typename T, int N>
int read_wrapped_sized_dependent_nested(
    Holder<typename SizedNestedOwner<T, N>::Inner> value) {
  return value.value.value + value.value.data[2];
}

template <typename T, int N>
int read_namespaced_sized_dependent_nested(
    typename nested_ns::SizedOwner<T, N>::Inner value) {
  return value.value + value.data[2];
}

template <typename T, int N>
int read_wrapped_namespaced_sized_dependent_nested(
    Holder<typename nested_ns::SizedOwner<T, N>::Inner> value) {
  return value.value.value + value.value.data[2];
}

template <typename T, int N>
typename nested_ns::SizedOwner<T, N>::Inner
make_namespaced_sized_dependent_nested(T value, int extra) {
  typename nested_ns::SizedOwner<T, N>::Inner result;
  result.value = value;
  result.data[2] = extra;
  return result;
}

template <typename T, int N>
Holder<typename nested_ns::SizedOwner<T, N>::Inner>
make_wrapped_namespaced_sized_dependent_nested(T value, int extra) {
  Holder<typename nested_ns::SizedOwner<T, N>::Inner> result;
  result.value.value = value;
  result.value.data[2] = extra;
  return result;
}

template <typename T, int N>
int read_namespaced_sized_dependent_nested_pointer(
    typename nested_ns::SizedOwner<T, N>::Inner* value) {
  return value->value + value->data[2];
}

template <typename T, int N>
int read_namespaced_sized_dependent_nested_reference(
    typename nested_ns::SizedOwner<T, N>::Inner& value) {
  return value.value + value.data[2];
}

template <typename T, int N>
int read_wrapped_namespaced_sized_dependent_nested_pointer(
    Holder<typename nested_ns::SizedOwner<T, N>::Inner>* value) {
  return value->value.value + value->value.data[2];
}

template <typename T, int N>
int read_wrapped_namespaced_sized_dependent_nested_reference(
    Holder<typename nested_ns::SizedOwner<T, N>::Inner>& value) {
  return value.value.value + value.value.data[2];
}

template <typename T, int N>
int read_const_namespaced_sized_dependent_nested_pointer(
    const typename nested_ns::SizedOwner<T, N>::Inner* value) {
  return value->value + value->data[2];
}

template <typename T, int N>
int read_const_namespaced_sized_dependent_nested_reference(
    const typename nested_ns::SizedOwner<T, N>::Inner& value) {
  return value.value + value.data[2];
}

template <typename T, int N>
int read_const_wrapped_namespaced_sized_dependent_nested_pointer(
    const Holder<typename nested_ns::SizedOwner<T, N>::Inner>* value) {
  return value->value.value + value->value.data[2];
}

template <typename T, int N>
int read_const_wrapped_namespaced_sized_dependent_nested_reference(
    const Holder<typename nested_ns::SizedOwner<T, N>::Inner>& value) {
  return value.value.value + value.value.data[2];
}

template <typename T, int N>
int read_namespaced_sized_dependent_nested_array(
    typename nested_ns::SizedOwner<T, N>::Inner value[1]) {
  return value[0].value + value[0].data[2];
}

template <typename T, int N>
int read_wrapped_namespaced_sized_dependent_nested_array(
    Holder<typename nested_ns::SizedOwner<T, N>::Inner> value[1]) {
  return value[0].value.value + value[0].value.data[2];
}

template <typename T, int N>
Holder<typename SizedNestedOwner<T, N>::Inner>
make_wrapped_sized_dependent_nested(T value, int extra) {
  Holder<typename SizedNestedOwner<T, N>::Inner> result;
  result.value.value = value;
  result.value.data[2] = extra;
  return result;
}

template <typename T, int N>
int read_wrapped_sized_dependent_nested_pointer(
    Holder<typename SizedNestedOwner<T, N>::Inner>* value) {
  return value->value.value + value->value.data[2];
}

template <typename T, int N>
int read_wrapped_sized_dependent_nested_reference(
    Holder<typename SizedNestedOwner<T, N>::Inner>& value) {
  return value.value.value + value.value.data[2];
}

template <typename T, int N>
int read_sized_dependent_nested_pointer(
    typename SizedNestedOwner<T, N>::Inner* value) {
  return value->value + value->data[2];
}

template <typename T, int N>
int read_sized_dependent_nested_reference(
    typename SizedNestedOwner<T, N>::Inner& value) {
  return value.value + value.data[2];
}

template <typename T, int N>
int read_const_sized_dependent_nested_pointer(
    const typename SizedNestedOwner<T, N>::Inner* value) {
  return value->value + value->data[2];
}

template <typename T, int N>
int read_const_sized_dependent_nested_reference(
    const typename SizedNestedOwner<T, N>::Inner& value) {
  return value.value + value.data[2];
}

template <typename T, int N>
int read_const_wrapped_sized_dependent_nested_pointer(
    const Holder<typename SizedNestedOwner<T, N>::Inner>* value) {
  return value->value.value + value->value.data[2];
}

template <typename T, int N>
int read_const_wrapped_sized_dependent_nested_reference(
    const Holder<typename SizedNestedOwner<T, N>::Inner>& value) {
  return value.value.value + value.value.data[2];
}

template <typename T, int N>
int read_sized_dependent_nested_array(
    typename SizedNestedOwner<T, N>::Inner value[1]) {
  return value[0].value + value[0].data[2];
}

template <typename T, int N>
int read_sized_member_owner_array(SizedMemberOwner<T, N> owners[1]) {
  return owners[0].stored.value + owners[0].stored.data[2];
}

template <typename T, int N>
int read_namespaced_sized_member_owner_array(
    nested_ns::SizedMemberOwner<T, N> owners[1]) {
  return owners[0].stored.value + owners[0].stored.data[2];
}

template <typename T, int N>
int read_wrapped_sized_dependent_nested_array(
    Holder<typename SizedNestedOwner<T, N>::Inner> value[1]) {
  return value[0].value.value + value[0].value.data[2];
}

template <typename T>
T read_dependent_nested_pointer(typename NestedOwner<T>::Inner* value) {
  return value->value;
}

template <typename T>
T read_dependent_nested_reference(typename NestedOwner<T>::Inner& value) {
  return value.value;
}

template <typename T>
T read_const_dependent_nested_pointer(
    const typename NestedOwner<T>::Inner* value) {
  return value->value;
}

template <typename T>
T read_const_dependent_nested_reference(
    const typename NestedOwner<T>::Inner& value) {
  return value.value;
}

template <typename T>
T read_dependent_nested_array(typename NestedOwner<T>::Inner value[1]) {
  return value[0].value;
}

template <typename T>
T read_namespaced_dependent_nested_pointer(
    typename nested_ns::Owner<T>::Inner* value) {
  return value->value;
}

template <typename T>
T read_namespaced_dependent_nested_reference(
    typename nested_ns::Owner<T>::Inner& value) {
  return value.value;
}

template <typename T>
T read_const_namespaced_dependent_nested_pointer(
    const typename nested_ns::Owner<T>::Inner* value) {
  return value->value;
}

template <typename T>
T read_const_namespaced_dependent_nested_reference(
    const typename nested_ns::Owner<T>::Inner& value) {
  return value.value;
}

template <typename T>
T read_namespaced_dependent_nested_array(
    typename nested_ns::Owner<T>::Inner value[1]) {
  return value[0].value;
}

template <typename T>
T read_wrapped_dependent_nested(Holder<typename NestedOwner<T>::Inner> value) {
  return value.value.value;
}

template <typename T>
T read_wrapped_dependent_nested_pointer(
    Holder<typename NestedOwner<T>::Inner>* value) {
  return value->value.value;
}

template <typename T>
T read_wrapped_dependent_nested_reference(
    Holder<typename NestedOwner<T>::Inner>& value) {
  return value.value.value;
}

template <typename T>
T read_const_wrapped_dependent_nested_pointer(
    const Holder<typename NestedOwner<T>::Inner>* value) {
  return value->value.value;
}

template <typename T>
T read_const_wrapped_dependent_nested_reference(
    const Holder<typename NestedOwner<T>::Inner>& value) {
  return value.value.value;
}

template <typename T>
T read_wrapped_dependent_nested_array(
    Holder<typename NestedOwner<T>::Inner> value[1]) {
  return value[0].value.value;
}

template <typename T>
Holder<typename NestedOwner<T>::Inner> make_wrapped_dependent_nested(T value) {
  Holder<typename NestedOwner<T>::Inner> result;
  result.value.value = value;
  return result;
}

template <typename T>
Holder<typename nested_ns::Owner<T>::Inner>
make_wrapped_namespaced_dependent_nested(T value) {
  Holder<typename nested_ns::Owner<T>::Inner> result;
  result.value.value = value;
  return result;
}

template <typename T>
T read_wrapped_namespaced_dependent_nested(
    Holder<typename nested_ns::Owner<T>::Inner> value) {
  return value.value.value;
}

template <typename T>
T read_wrapped_namespaced_dependent_pointer(
    Holder<typename nested_ns::Owner<T>::Inner>* value) {
  return value->value.value;
}

template <typename T>
T read_wrapped_namespaced_dependent_reference(
    Holder<typename nested_ns::Owner<T>::Inner>& value) {
  return value.value.value;
}

template <typename T>
T read_const_wrapped_namespaced_dependent_pointer(
    const Holder<typename nested_ns::Owner<T>::Inner>* value) {
  return value->value.value;
}

template <typename T>
T read_const_wrapped_namespaced_dependent_reference(
    const Holder<typename nested_ns::Owner<T>::Inner>& value) {
  return value.value.value;
}

template <typename T>
T read_wrapped_namespaced_dependent_array(
    Holder<typename nested_ns::Owner<T>::Inner> value[1]) {
  return value[0].value.value;
}

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
  specialized_holder.specialized_static_value = 41;
  specialized_holder.specialized_static_value += 1;
  default_specialized_holder.default_static_value = 44;
  default_specialized_holder.default_static_value += 2;
  specialized_box.box_static_value = 50;
  specialized_box.box_static_value += 3;
  int specialized_static_post = specialized_holder.specialized_static_value++;
  int specialized_static_pre = ++specialized_holder.specialized_static_value;
  int default_static_post = default_specialized_holder.default_static_value--;
  int default_static_pre = --default_specialized_holder.default_static_value;
  long box_static_post = specialized_box.box_static_value++;
  long box_static_pre = ++specialized_box.box_static_value;
  SpecializedHolder<int>* specialized_holder_ptr = &specialized_holder;
  DefaultSpecializedHolder<>* default_specialized_holder_ptr =
      &default_specialized_holder;
  cache::Box<long>* specialized_box_ptr = &specialized_box;
  int specialized_arrow_static_post =
      specialized_holder_ptr->specialized_static_value++;
  int specialized_arrow_static_pre =
      ++specialized_holder_ptr->specialized_static_value;
  int specialized_arrow_static_total =
      specialized_holder_ptr->specialized_static_total();
  int default_arrow_static_post =
      default_specialized_holder_ptr->default_static_value--;
  int default_arrow_static_pre =
      --default_specialized_holder_ptr->default_static_value;
  int default_arrow_static_total =
      default_specialized_holder_ptr->default_static_total();
  long box_arrow_static_post = specialized_box_ptr->box_static_value++;
  long box_arrow_static_pre = ++specialized_box_ptr->box_static_value;
  long box_arrow_static_total = specialized_box_ptr->box_static_total();
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
  NestedOwner<int>::Inner nested_inner;
  nested_inner.value = 11;
  NestedOwner<char>::Inner nested_char_inner;
  nested_char_inner.value = 'x';
  NestedOwner<long>::Inner nested_long_inner;
  nested_long_inner.value = 123;
  nested_ns::Owner<int>::Inner namespaced_nested_inner;
  namespaced_nested_inner.value = 17;
  NestedOwner<int>::Inner signature_nested_inner = make_nested_inner();
  int signature_nested_value = read_nested_inner(signature_nested_inner);
  Holder<NestedOwner<int>::Inner> wrapped_nested_inner;
  wrapped_nested_inner.value.value = 7;
  Holder<nested_ns::Owner<int>::Inner> wrapped_namespaced_nested_inner;
  wrapped_namespaced_nested_inner.value.value = 23;
  UsesNested<int> uses_nested;
  uses_nested.value.value = 29;
  UsesNamespacedNested<int> uses_namespaced_nested;
  uses_namespaced_nested.value.value = 31;
  UsesWrappedNested<int> uses_wrapped_nested;
  uses_wrapped_nested.value.value.value = 37;
  UsesSizedNested<int, 3> uses_sized_nested;
  uses_sized_nested.value.value = 127;
  uses_sized_nested.value.data[2] = 18;
  UsesWrappedSizedNested<int, 3> uses_wrapped_sized_nested;
  uses_wrapped_sized_nested.value.value.value = 131;
  uses_wrapped_sized_nested.value.value.data[2] = 20;
  NestedOwner<int>::Inner dependent_signature_nested =
      make_dependent_nested<int>(41);
  int dependent_signature_value =
      read_dependent_nested<int>(dependent_signature_nested);
  NestedOwner<int>::Inner deduced_signature_nested = make_dependent_nested(43);
  int deduced_signature_value = read_dependent_nested(deduced_signature_nested);
  PairNestedOwner<int, char>::Inner pair_dependent_nested;
  pair_dependent_nested.first = 73;
  pair_dependent_nested.second = 5;
  int pair_dependent_nested_value =
      read_pair_dependent_nested(pair_dependent_nested);
  SizedNestedOwner<int, 3>::Inner sized_dependent_nested;
  sized_dependent_nested.value = 79;
  sized_dependent_nested.data[2] = 6;
  int sized_dependent_nested_value =
      read_sized_dependent_nested(sized_dependent_nested);
  Holder<SizedNestedOwner<int, 3>::Inner> wrapped_sized_dependent_nested;
  wrapped_sized_dependent_nested.value.value = 83;
  wrapped_sized_dependent_nested.value.data[2] = 8;
  int wrapped_sized_dependent_nested_value =
      read_wrapped_sized_dependent_nested(wrapped_sized_dependent_nested);
  int sized_dependent_pointer_value =
      read_sized_dependent_nested_pointer(&sized_dependent_nested);
  int sized_dependent_reference_value =
      read_sized_dependent_nested_reference(sized_dependent_nested);
  int wrapped_sized_dependent_pointer_value =
      read_wrapped_sized_dependent_nested_pointer(
          &wrapped_sized_dependent_nested);
  int wrapped_sized_dependent_reference_value =
      read_wrapped_sized_dependent_nested_reference(
          wrapped_sized_dependent_nested);
  int const_sized_dependent_pointer_value =
      read_const_sized_dependent_nested_pointer(&sized_dependent_nested);
  int const_sized_dependent_reference_value =
      read_const_sized_dependent_nested_reference(sized_dependent_nested);
  int const_wrapped_sized_dependent_pointer_value =
      read_const_wrapped_sized_dependent_nested_pointer(
          &wrapped_sized_dependent_nested);
  int const_wrapped_sized_dependent_reference_value =
      read_const_wrapped_sized_dependent_nested_reference(
          wrapped_sized_dependent_nested);
  SizedNestedOwner<int, 3>::Inner sized_dependent_array[1];
  sized_dependent_array[0].value = 89;
  sized_dependent_array[0].data[2] = 10;
  int sized_dependent_array_value =
      read_sized_dependent_nested_array(sized_dependent_array);
  Holder<SizedNestedOwner<int, 3>::Inner> wrapped_sized_dependent_array[1];
  wrapped_sized_dependent_array[0].value.value = 97;
  wrapped_sized_dependent_array[0].value.data[2] = 12;
  int wrapped_sized_dependent_array_value =
      read_wrapped_sized_dependent_nested_array(
          wrapped_sized_dependent_array);
  SizedMemberOwner<int, 3> sized_member_owner_array[1];
  sized_member_owner_array[0].stored.value = 181;
  sized_member_owner_array[0].stored.data[2] = 38;
  int sized_member_owner_array_value =
      read_sized_member_owner_array(sized_member_owner_array);
  SizedNestedOwner<int, 3>::Inner made_sized_dependent_nested =
      make_sized_dependent_nested<int, 3>(101, 14);
  int made_sized_dependent_nested_value =
      read_sized_dependent_nested(made_sized_dependent_nested);
  Holder<SizedNestedOwner<int, 3>::Inner> made_wrapped_sized_dependent_nested =
      make_wrapped_sized_dependent_nested<int, 3>(117, 16);
  int made_wrapped_sized_dependent_nested_value =
      read_wrapped_sized_dependent_nested(made_wrapped_sized_dependent_nested);
  nested_ns::SizedOwner<int, 3>::Inner namespaced_sized_dependent_nested;
  namespaced_sized_dependent_nested.value = 137;
  namespaced_sized_dependent_nested.data[2] = 22;
  int namespaced_sized_dependent_nested_value =
      read_namespaced_sized_dependent_nested(
          namespaced_sized_dependent_nested);
  Holder<nested_ns::SizedOwner<int, 3>::Inner>
      wrapped_namespaced_sized_dependent_nested;
  wrapped_namespaced_sized_dependent_nested.value.value = 139;
  wrapped_namespaced_sized_dependent_nested.value.data[2] = 24;
  int wrapped_namespaced_sized_dependent_nested_value =
      read_wrapped_namespaced_sized_dependent_nested(
          wrapped_namespaced_sized_dependent_nested);
  int namespaced_sized_dependent_pointer_value =
      read_namespaced_sized_dependent_nested_pointer(
          &namespaced_sized_dependent_nested);
  int namespaced_sized_dependent_reference_value =
      read_namespaced_sized_dependent_nested_reference(
          namespaced_sized_dependent_nested);
  int wrapped_namespaced_sized_dependent_pointer_value =
      read_wrapped_namespaced_sized_dependent_nested_pointer(
          &wrapped_namespaced_sized_dependent_nested);
  int wrapped_namespaced_sized_dependent_reference_value =
      read_wrapped_namespaced_sized_dependent_nested_reference(
          wrapped_namespaced_sized_dependent_nested);
  int const_namespaced_sized_dependent_pointer_value =
      read_const_namespaced_sized_dependent_nested_pointer(
          &namespaced_sized_dependent_nested);
  int const_namespaced_sized_dependent_reference_value =
      read_const_namespaced_sized_dependent_nested_reference(
          namespaced_sized_dependent_nested);
  int const_wrapped_namespaced_sized_dependent_pointer_value =
      read_const_wrapped_namespaced_sized_dependent_nested_pointer(
          &wrapped_namespaced_sized_dependent_nested);
  int const_wrapped_namespaced_sized_dependent_reference_value =
      read_const_wrapped_namespaced_sized_dependent_nested_reference(
          wrapped_namespaced_sized_dependent_nested);
  nested_ns::SizedOwner<int, 3>::Inner namespaced_sized_dependent_array[1];
  namespaced_sized_dependent_array[0].value = 149;
  namespaced_sized_dependent_array[0].data[2] = 26;
  int namespaced_sized_dependent_array_value =
      read_namespaced_sized_dependent_nested_array(
          namespaced_sized_dependent_array);
  Holder<nested_ns::SizedOwner<int, 3>::Inner>
      wrapped_namespaced_sized_dependent_array[1];
  wrapped_namespaced_sized_dependent_array[0].value.value = 151;
  wrapped_namespaced_sized_dependent_array[0].value.data[2] = 28;
  int wrapped_namespaced_sized_dependent_array_value =
      read_wrapped_namespaced_sized_dependent_nested_array(
          wrapped_namespaced_sized_dependent_array);
  nested_ns::SizedMemberOwner<int, 3>
      namespaced_sized_member_owner_array[1];
  namespaced_sized_member_owner_array[0].stored.value = 191;
  namespaced_sized_member_owner_array[0].stored.data[2] = 40;
  int namespaced_sized_member_owner_array_value =
      read_namespaced_sized_member_owner_array(
          namespaced_sized_member_owner_array);
  nested_ns::SizedOwner<int, 3>::Inner made_namespaced_sized_dependent_nested =
      make_namespaced_sized_dependent_nested<int, 3>(167, 34);
  int made_namespaced_sized_dependent_nested_value =
      read_namespaced_sized_dependent_nested(
          made_namespaced_sized_dependent_nested);
  Holder<nested_ns::SizedOwner<int, 3>::Inner>
      made_wrapped_namespaced_sized_dependent_nested =
          make_wrapped_namespaced_sized_dependent_nested<int, 3>(173, 36);
  int made_wrapped_namespaced_sized_dependent_nested_value =
      read_wrapped_namespaced_sized_dependent_nested(
          made_wrapped_namespaced_sized_dependent_nested);
  int dependent_pointer_value =
      read_dependent_nested_pointer(&deduced_signature_nested);
  int dependent_reference_value =
      read_dependent_nested_reference(deduced_signature_nested);
  int const_dependent_pointer_value =
      read_const_dependent_nested_pointer(&deduced_signature_nested);
  int const_dependent_reference_value =
      read_const_dependent_nested_reference(deduced_signature_nested);
  NestedOwner<int>::Inner dependent_nested_array[1];
  dependent_nested_array[0].value = 59;
  int dependent_array_value =
      read_dependent_nested_array(dependent_nested_array);
  int namespaced_dependent_pointer_value =
      read_namespaced_dependent_nested_pointer(&namespaced_nested_inner);
  int namespaced_dependent_reference_value =
      read_namespaced_dependent_nested_reference(namespaced_nested_inner);
  int const_namespaced_dependent_pointer_value =
      read_const_namespaced_dependent_nested_pointer(&namespaced_nested_inner);
  int const_namespaced_dependent_reference_value =
      read_const_namespaced_dependent_nested_reference(namespaced_nested_inner);
  nested_ns::Owner<int>::Inner namespaced_dependent_array[1];
  namespaced_dependent_array[0].value = 67;
  int namespaced_dependent_array_value =
      read_namespaced_dependent_nested_array(namespaced_dependent_array);
  int wrapped_dependent_value =
      read_wrapped_dependent_nested(wrapped_nested_inner);
  int wrapped_dependent_pointer_value =
      read_wrapped_dependent_nested_pointer(&wrapped_nested_inner);
  int wrapped_dependent_reference_value =
      read_wrapped_dependent_nested_reference(wrapped_nested_inner);
  int const_wrapped_dependent_pointer_value =
      read_const_wrapped_dependent_nested_pointer(&wrapped_nested_inner);
  int const_wrapped_dependent_reference_value =
      read_const_wrapped_dependent_nested_reference(wrapped_nested_inner);
  Holder<NestedOwner<int>::Inner> wrapped_dependent_array[1];
  wrapped_dependent_array[0].value.value = 61;
  int wrapped_dependent_array_value =
      read_wrapped_dependent_nested_array(wrapped_dependent_array);
  Holder<NestedOwner<int>::Inner> made_wrapped_dependent_nested =
      make_wrapped_dependent_nested(47);
  Holder<nested_ns::Owner<int>::Inner> made_wrapped_namespaced_dependent_nested =
      make_wrapped_namespaced_dependent_nested(53);
  int wrapped_namespaced_dependent_value =
      read_wrapped_namespaced_dependent_nested(wrapped_namespaced_nested_inner);
  int wrapped_namespaced_dependent_pointer_value =
      read_wrapped_namespaced_dependent_pointer(
          &wrapped_namespaced_nested_inner);
  int wrapped_namespaced_dependent_reference_value =
      read_wrapped_namespaced_dependent_reference(
          wrapped_namespaced_nested_inner);
  int const_wrapped_namespaced_dependent_pointer_value =
      read_const_wrapped_namespaced_dependent_pointer(
          &wrapped_namespaced_nested_inner);
  int const_wrapped_namespaced_dependent_reference_value =
      read_const_wrapped_namespaced_dependent_reference(
          wrapped_namespaced_nested_inner);
  Holder<nested_ns::Owner<int>::Inner> wrapped_namespaced_dependent_array[1];
  wrapped_namespaced_dependent_array[0].value.value = 71;
  int wrapped_namespaced_dependent_array_value =
      read_wrapped_namespaced_dependent_array(
          wrapped_namespaced_dependent_array);
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
         nested_inner.value + nested_char_inner.value +
         nested_long_inner.value + namespaced_nested_inner.value +
         signature_nested_inner.value + signature_nested_value +
         wrapped_nested_inner.value.value +
         wrapped_namespaced_nested_inner.value.value +
         uses_nested.value.value +
         uses_namespaced_nested.value.value +
         uses_wrapped_nested.value.value.value +
         uses_sized_nested.value.value + uses_sized_nested.value.data[2] +
         uses_wrapped_sized_nested.value.value.value +
         uses_wrapped_sized_nested.value.value.data[2] +
         dependent_signature_nested.value + dependent_signature_value +
         deduced_signature_nested.value + deduced_signature_value +
         pair_dependent_nested_value +
         sized_dependent_nested_value +
         wrapped_sized_dependent_nested_value +
         sized_dependent_pointer_value + sized_dependent_reference_value +
         wrapped_sized_dependent_pointer_value +
         wrapped_sized_dependent_reference_value +
         const_sized_dependent_pointer_value +
         const_sized_dependent_reference_value +
         const_wrapped_sized_dependent_pointer_value +
         const_wrapped_sized_dependent_reference_value +
         sized_dependent_array_value + wrapped_sized_dependent_array_value +
         sized_member_owner_array_value +
         made_sized_dependent_nested_value +
         made_wrapped_sized_dependent_nested_value +
         namespaced_sized_dependent_nested_value +
         wrapped_namespaced_sized_dependent_nested_value +
         namespaced_sized_dependent_pointer_value +
         namespaced_sized_dependent_reference_value +
         wrapped_namespaced_sized_dependent_pointer_value +
         wrapped_namespaced_sized_dependent_reference_value +
         const_namespaced_sized_dependent_pointer_value +
         const_namespaced_sized_dependent_reference_value +
         const_wrapped_namespaced_sized_dependent_pointer_value +
         const_wrapped_namespaced_sized_dependent_reference_value +
         namespaced_sized_dependent_array_value +
         wrapped_namespaced_sized_dependent_array_value +
         namespaced_sized_member_owner_array_value +
         made_namespaced_sized_dependent_nested_value +
         made_wrapped_namespaced_sized_dependent_nested_value +
         dependent_pointer_value + dependent_reference_value +
         const_dependent_pointer_value + const_dependent_reference_value +
         dependent_array_value +
         namespaced_dependent_pointer_value +
         namespaced_dependent_reference_value +
         const_namespaced_dependent_pointer_value +
         const_namespaced_dependent_reference_value +
         namespaced_dependent_array_value +
         wrapped_dependent_value +
         wrapped_dependent_pointer_value +
         wrapped_dependent_reference_value +
         const_wrapped_dependent_pointer_value +
         const_wrapped_dependent_reference_value +
         wrapped_dependent_array_value +
         made_wrapped_dependent_nested.value.value +
         made_wrapped_namespaced_dependent_nested.value.value +
         wrapped_namespaced_dependent_value +
         wrapped_namespaced_dependent_pointer_value +
         wrapped_namespaced_dependent_reference_value +
         const_wrapped_namespaced_dependent_pointer_value +
         const_wrapped_namespaced_dependent_reference_value +
         wrapped_namespaced_dependent_array_value +
         default_buffer.data[3] + explicit_default_buffer.data[1] +
         default_matrix.data[3] + explicit_matrix.data[2] +
         specialized_holder.value + specialized_holder.bonus +
         specialized_holder.get_bonus() +
         specialized_holder.inline_total() +
         SpecializedHolder<int>::specialized_static_value +
         SpecializedHolder<int>::specialized_static_total() +
         specialized_holder.specialized_static_value +
         specialized_holder.specialized_static_total() +
         specialized_static_post + specialized_static_pre +
         specialized_holder_ptr->specialized_static_value +
         specialized_holder_ptr->specialized_static_total() +
         specialized_arrow_static_post + specialized_arrow_static_pre +
         specialized_arrow_static_total +
         primary_holder.value + declared_specialized_holder.value +
         declared_specialized_holder.bonus +
         declared_specialized_holder.sum() + declared_primary_holder.value +
         default_specialized_holder.value + default_specialized_holder.bonus +
         default_specialized_holder.sum() +
         default_specialized_holder.inline_total() +
         DefaultSpecializedHolder<>::default_static_value +
         DefaultSpecializedHolder<>::default_static_total() +
         default_specialized_holder.default_static_value +
         default_specialized_holder.default_static_total() +
         default_static_post + default_static_pre +
         default_specialized_holder_ptr->default_static_value +
         default_specialized_holder_ptr->default_static_total() +
         default_arrow_static_post + default_arrow_static_pre +
         default_arrow_static_total +
         default_primary_holder.value + specialized_lifecycle_trace +
         specialized_box.bonus +
         specialized_box.sum() +
         specialized_box.inline_sum() +
         cache::Box<long>::box_static_value +
         cache::Box<long>::box_static_total() +
         specialized_box.box_static_value +
         specialized_box.box_static_total() +
         box_static_post + box_static_pre +
         specialized_box_ptr->box_static_value +
         specialized_box_ptr->box_static_total() + box_arrow_static_post +
         box_arrow_static_pre + box_arrow_static_total +
         left_box.value.left_value + right_box.value.right_value +
         plain_result;
}
