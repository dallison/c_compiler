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

int main(void) {
  Holder<int> local;
  Holder<int> again;
  local.value = 42;
  again.value = local.value;
  Pair<int, char> pair;
  pair.first = again.value;
  pair.second = 8;
  Buffer<4> buffer;
  Buffer<4> same_buffer;
  Buffer<2 + 2> expression_buffer;
  Buffer<8> bigger_buffer;
  WithMemberFunction<int> member_function_holder;
  WithMemberFunction<char> char_member_function_holder;
  WithMemberTemplate<int> member_template_holder;
  WithDependentMember<int, 4> dependent_member_holder;
  WithDependentMember<int, 4> constructed_member(9);
  WithOutOfClassCtor<int> out_of_class_constructed(11);
  enum { enum_count = 4 };
  Buffer<enum_count> enum_buffer;
  const int const_count = 4;
  Buffer<const_count> const_buffer;
  buffer.data[0] = pair.first;
  buffer.data[3] = pair.second;
  same_buffer.data[3] = buffer.data[3];
  expression_buffer.data[3] = same_buffer.data[3];
  enum_buffer.data[3] = expression_buffer.data[3];
  const_buffer.data[3] = enum_buffer.data[3];
  bigger_buffer.data[7] = const_buffer.data[3];
  cache::Box<int> box;
  box.value = bigger_buffer.data[7];
  member_function_holder.set(box.value);
  char_member_function_holder.set(5);
  Holder<int> wrapped_member = member_function_holder.wrap(3);
  Holder<char> wrapped_char_member = char_member_function_holder.wrap(2);
  Holder<IntAlias> alias_holder;
  Holder<int> inline_wrapped_member = member_function_holder.inline_wrap(6);
  alias_holder.value = member_function_holder.inline_add(
      member_function_holder.unwrap(wrapped_member)) +
      member_function_holder.unwrap(inline_wrapped_member);
  char deduced_member_template = member_template_holder.choose(9, 'a');
  char explicit_member_template =
      member_template_holder.explicit_choose<char>(10, 'b');
  dependent_member_holder.stored.value = alias_holder.value;
  dependent_member_holder.buffer.data[3] =
      dependent_member_holder.stored.value + 4;
  constructed_member.buffer.data[3] = constructed_member.buffer.data[0] + 1;
  out_of_class_constructed.marker =
      out_of_class_constructed.stored.value + out_of_class_constructed.marker;
  {
    WithOutOfClassDtor<int> destroyed_at_block_exit(13);
    template_destructor_trace = destroyed_at_block_exit.value - 13;
  }
  int pointed = 1;
  Holder<int*> pointer_holder;
  pointer_holder.value = &pointed;
  cache::Box<left::Item> left_box;
  cache::Box<right::Item> right_box;
  left_box.value.left_value = *pointer_holder.value;
  right_box.value.right_value = 2;
  InlinePlain plain;
  plain.value = 4;
  int plain_result = plain.bump(3);
  return buffer.data[0] + alias_holder.value + left_box.value.left_value +
         right_box.value.right_value +
         char_member_function_holder.unwrap(wrapped_char_member) +
         deduced_member_template + explicit_member_template +
         plain_result + dependent_member_holder.buffer.data[3] +
         constructed_member.buffer.data[3] + out_of_class_constructed.marker -
         320 + template_destructor_trace - 13;
}
