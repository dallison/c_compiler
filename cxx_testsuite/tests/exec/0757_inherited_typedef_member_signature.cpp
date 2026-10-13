// RUN: -std=c++17
// EXPECT_EXIT: 0

template <unsigned N, class T, class... Ts>
struct NthType : NthType<N - 1, Ts...> {};

template <class T, class... Ts>
struct NthType<0, T, Ts...> {
  using type = T;
};

template <class... Ts>
struct LayoutBase {
  template <unsigned N>
  using ElementType = typename NthType<N, Ts...>::type;
};

template <class... Ts>
struct LayoutMiddle : LayoutBase<Ts...> {};

template <class... Ts>
struct Layout : LayoutMiddle<Ts...> {};

template <class T>
struct SlotPolicy {
  using slot_type = T;
};

template <class Key, class A, class B, class C, class D, class Policy>
struct CommonParams {
  using slot_policy = Policy;
  using slot_type = typename slot_policy::slot_type;
};

template <class Key>
struct Params
    : CommonParams<Key, int, int, int, int, SlotPolicy<Key>> {
  using slot_type = typename Params::CommonParams::slot_type;

  template <class V>
  static const V& key(const V& value) {
    return value;
  }

  static const Key& key(const slot_type* slot) {
    return *slot;
  }
};

struct Pair {
  long first;
  long second;
};

template <class ParamsT>
struct Node {
  using slot_type = typename ParamsT::slot_type;
  using layout_type =
      Layout<Node*, unsigned, unsigned char, slot_type, Node*>;

  template <unsigned N>
  typename layout_type::template ElementType<N>* GetField() {
    return reinterpret_cast<
        typename layout_type::template ElementType<N>*>(&slot);
  }

  template <unsigned N>
  const typename layout_type::template ElementType<N>* GetField() const {
    return reinterpret_cast<
        const typename layout_type::template ElementType<N>*>(&slot);
  }

  const slot_type& value() const {
    return ParamsT::key(GetField<3>());
  }

  slot_type slot;
};

int main() {
  Node<Params<Pair>> node{{4, 5}};
  return node.value().first + node.value().second == 9 ? 0 : 1;
}
