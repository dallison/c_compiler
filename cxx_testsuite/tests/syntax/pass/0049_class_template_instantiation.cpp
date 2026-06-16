// RUN: -std=c++20

int template_destructor_trace;

template <typename T>
struct Holder {
  T value;
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

template <int N>
struct Buffer {
  int data[N];
};

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
  WithMemberFunction<int> member_function_holder;
  WithMemberFunction<char> char_member_function_holder;
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
  cache::Box<int> box;
  box.value = bigger_buffer.data[7];
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
  cache::Box<left::Item> left_box;
  cache::Box<right::Item> right_box;
  left_box.value.left_value = 1;
  right_box.value.right_value = 2;
  InlinePlain plain;
  plain.value = 4;
  int plain_result = plain.bump(3);
  return pair.first + box.value + left_box.value.left_value +
         right_box.value.right_value + plain_result;
}
