// `typename alias::member` where the alias is itself a member of a dependent
// specialization (`using proxy = typename cond<B, T, U>::type`) names
// `cond<B, T, U>::type::member`, not `cond<B, T, U>::member`.

template <class T, class U>
struct same {
  static const bool value = false;
};
template <class T>
struct same<T, T> {
  static const bool value = true;
};

template <bool B, class T, class F>
struct cond {
  using type = T;
  using other = T;
};
template <class T, class F>
struct cond<false, T, F> {
  using type = F;
  using other = F;
};

template <typename Head_, typename... Tail_>
struct Types {
  using Head = Head_;
};
template <typename... Ts>
struct ProxyTypeList {
  using type = Types<Ts...>;
};
template <typename>
struct is_proxy_type_list {
  static const bool value = false;
};
template <typename... Ts>
struct is_proxy_type_list<ProxyTypeList<Ts...>> {
  static const bool value = true;
};

// googletest's GenerateTypeList.
template <typename T>
struct GenerateTypeList {
 private:
  using proxy = typename cond<is_proxy_type_list<T>::value, T,
                              ProxyTypeList<T>>::type;

 public:
  using type = typename proxy::type;
};

// The alias's member has a different name from the one looked up through it.
template <typename T>
struct ViaOther {
  using proxy = typename cond<true, T, int>::other;
  using result = typename proxy::type;
};

struct Leaf {
  int value;
};
struct Mid {
  using inner = Leaf;
};
struct Outer {
  using type = Mid;
};

// Two components through the alias.
template <typename T>
struct Deep {
  using proxy = typename cond<true, T, int>::type;
  using result = typename proxy::type::inner;
};

int Use(Types<char, int>::Head c) { return c; }

int main() {
  typedef GenerateTypeList<ProxyTypeList<char, int>>::type L;
  if (!same<L, Types<char, int>>::value) return 1;
  if (!same<GenerateTypeList<int>::type, Types<int>>::value) return 2;
  if (!same<ViaOther<ProxyTypeList<char>>::result, Types<char>>::value)
    return 3;
  if (!same<Deep<Outer>::result, Leaf>::value) return 4;

  // Used directly, not only through a template argument.
  L::Head head = 'a';
  if (sizeof(head) != 1) return 5;
  Deep<Outer>::result leaf = {7};
  if (leaf.value != 7) return 6;
  return Use(head) == 'a' ? 0 : 7;
}
