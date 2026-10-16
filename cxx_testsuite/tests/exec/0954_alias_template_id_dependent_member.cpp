// RUN: -std=c++17
// A member of a dependent alias template-id, `typename A_t<M>::KeyNode` where
// `A_t<T>` is `typename A<T>::type`, names `A<M>::type::KeyNode`.  Expanding
// the alias must not drop the member: the result is the nested type, not the
// class that contains it.
template <class T> struct A { using type = T; };
template <class T> using A_t = typename A<T>::type;
template <class T> struct B { using type = T; };
template <class T> struct B<T&> { using type = T; };
template <class T> using B_t = typename B<T>::type;
template <class T> struct C { using type = T; };
template <class T> struct C<const T> { using type = T; };
template <class T> using C_t = typename C<T>::type;
template <class T> using BA_t = A_t<B_t<T>>;

template <class K> struct Node {
  using value_type = K;
  K key;
};
template <class K> struct Map {
  using KeyNode = Node<K>;
  template <class U> struct rebind { using other = U*; };
};

template <class M> int ViaAlias(M&) {
  typename A_t<M>::KeyNode n{1};
  return n.key;
}
template <class M> int ViaPartialSpecialization(M&) {
  typename B_t<M&>::KeyNode n{2};
  return n.key;
}
template <class M> int ViaConstPartialSpecialization(M&) {
  typename C_t<const M>::KeyNode n{3};
  return n.key;
}
template <class M> int ViaLongerPath(M&) {
  typename A_t<M>::KeyNode::value_type v = 4;
  return v;
}
template <class M> int ViaLocalAlias(M&) {
  using Base = B_t<M&>;
  typename Base::KeyNode n{5};
  return n.key;
}
template <class M> int ViaMemberTemplate(M&) {
  typename A_t<M>::template rebind<long>::other p = nullptr;
  return sizeof(*p) == sizeof(long) ? 11 : 0;
}
template <class M> int ViaAliasOfAlias(M&) {
  typename BA_t<M&>::KeyNode n{6};
  return n.key;
}

struct NodeBase {};
template <class K> struct DerivedNode : NodeBase { K key = 7; };
template <class K> struct NodeMap { using KeyNode = DerivedNode<K>; };

int main() {
  Map<int> m;
  if (ViaAlias(m) != 1) return 1;
  if (ViaPartialSpecialization(m) != 2) return 2;
  if (ViaConstPartialSpecialization(m) != 3) return 3;
  if (ViaLongerPath(m) != 4) return 4;
  if (ViaLocalAlias(m) != 5) return 5;
  if (ViaAliasOfAlias(m) != 6) return 6;
  if (ViaMemberTemplate(m) != 11) return 11;

  auto direct = [](auto& map) {
    typename B_t<decltype(map)>::KeyNode n{8};
    return n.key;
  };
  if (direct(m) != 8) return 8;
  auto local = [](auto& map) {
    using Base = B_t<decltype(map)>;
    typename Base::KeyNode n{9};
    return n.key;
  };
  if (local(m) != 9) return 9;

  DerivedNode<long> node;
  NodeBase* base = &node;
  NodeMap<long> node_map;
  auto cast = [base](auto& map) {
    return static_cast<typename B_t<decltype(map)>::KeyNode*>(base)->key;
  };
  if (cast(node_map) != 7) return 10;
  return 0;
}
