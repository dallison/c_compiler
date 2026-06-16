// RUN: -std=c++20

template <typename T>
T identity(T value);

template <class T, typename U>
T choose_first(T first, U second);

namespace library {
template <typename T>
T namespaced_identity(T value);

template <typename T>
struct Box {
  T value;
};
}

template <typename T>
struct Holder {
  T value;
};

template <typename T>
Holder<T>* make_holder(void);

template <class T, typename U>
struct Pair {
  T first;
  U second;
};

template <int N>
struct Sized {
  int value;
};

int main(void) {
  return 0;
}
