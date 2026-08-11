// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0

template <class T>
struct remove_reference {
  using type = T;
};

template <class T>
struct remove_reference<T&> {
  using type = T;
};

template <class T>
using remove_reference_t = typename remove_reference<T>::type;

template <class T>
inline T&& pass(remove_reference_t<T>& value) {
  return static_cast<T&&>(value);
}

static int consume(char (&value)[3]) {
  return value[0] == 'o' ? 0 : 1;
}

template <class... Args>
static int consume_pack(Args&&... args) {
  return consume(pass<Args>(args)...);
}

template <class... Args>
static int relay_pack(Args&&... args) {
  return consume_pack(pass<Args>(args)...);
}

template <class T>
struct relay {
  template <class... Args>
  int consume_member(Args&&... args) {
    return consume(pass<Args>(args)...);
  }

  template <class... Args>
  int relay_member(Args&&... args) {
    return consume_member(pass<Args>(args)...);
  }
};

int main() {
  char value[] = "ok";
  if (consume(pass<char (&)[3]>(value)) != 0) {
    return 1;
  }
  if (relay_pack(value) != 0) {
    return 2;
  }
  relay<int> object;
  return object.relay_member(value);
}
