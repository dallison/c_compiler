// A template parameter hides a namespace-scope class template of the same name
// ([temp.local]/6): in `template <typename Types>`, `typename Types::Head` is a
// member of the argument, not of class template `Types` (googletest's
// GenerateNamesRecursively<NameGenerator, Types>).
struct None {};
template <typename Head_, typename... Tail_> struct Types {
  using Head = Head_;
  using Tail = Types<Tail_...>;
};
template <typename Head_> struct Types<Head_> {
  using Head = Head_;
  using Tail = None;
};

template <class T> struct Size {
  static const int value = sizeof(T);
};

template <typename Types> int HeadSize() {
  typedef typename Types::Head H;
  return sizeof(H);
}
template <typename Types> int HeadSizeViaTrait() {
  return Size<typename Types::Head>::value;
}

template <typename Types> int Sum(Types);
inline int Sum(None) { return 0; }
template <typename Types> int Sum(Types) {
  return sizeof(typename Types::Head) + Sum(typename Types::Tail());
}

int main() {
  if (HeadSize<Types<short, double>>() != 2) return 1;
  if (HeadSizeViaTrait<Types<short, double>>() != 2) return 2;
  if (Sum(Types<char, short, int>()) != 1 + 2 + 4) return 3;
  return 0;
}
