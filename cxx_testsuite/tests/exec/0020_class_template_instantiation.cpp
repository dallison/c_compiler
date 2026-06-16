template <typename T>
struct Holder {
  T value;
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
  Holder<IntAlias> alias_holder;
  alias_holder.value = box.value;
  int pointed = 1;
  Holder<int*> pointer_holder;
  pointer_holder.value = &pointed;
  cache::Box<left::Item> left_box;
  cache::Box<right::Item> right_box;
  left_box.value.left_value = *pointer_holder.value;
  right_box.value.right_value = 2;
  return buffer.data[0] + alias_holder.value + left_box.value.left_value +
         right_box.value.right_value - 53;
}
