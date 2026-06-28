// RUN: -std=c++20

template <class T>
struct PartialBox {
  T value;
  int kind(void) {
    return 1;
  }
};

template <class T>
struct PartialBox<T*> {
  T* value;
  PartialBox(T* initial) : value(initial) {
  }
  int kind(void) {
    return 2;
  }
  T& refer(void) {
    return *value;
  }
};

template <class T>
struct PartialBox<T&> {
  T* value;
  int kind(void) {
    return 3;
  }
};

template <class T, class U>
struct PartialPair {
  T first;
  U second;
  int kind(void) {
    return 4;
  }
};

template <class T, class U>
struct PartialPair<T*, U> {
  T* first;
  U second;
  int kind(void) {
    return 5;
  }
};

template <int N, int M>
struct PartialValue {
  int kind(void) {
    return 6;
  }
};

template <int N>
struct PartialValue<N, 0> {
  int kind(void) {
    return 7;
  }
};

template <class T>
struct ArrayPattern {
  int kind(void) {
    return 8;
  }
};

template <class T>
struct ArrayPattern<T[]> {
  int kind(void) {
    return 9;
  }
};

namespace partial_ns {
template <class T>
struct NamespacedPartial {
  int kind(void) {
    return 10;
  }
};

template <class T>
struct NamespacedPartial<T*> {
  int kind(void) {
    return 11;
  }
};
}

int main(void) {
  int value = 42;
  PartialBox<int> scalar;
  scalar.value = 1;
  PartialBox<int*> pointer(&value);
  PartialBox<int&> reference;
  reference.value = &value;
  PartialPair<int, char> pair_primary;
  PartialPair<int*, char> pair_pointer;
  PartialValue<2, 1> value_primary;
  PartialValue<2, 0> value_partial;
  ArrayPattern<int> array_primary;
  ArrayPattern<int[]> array_partial;
  partial_ns::NamespacedPartial<int> namespaced_primary;
  partial_ns::NamespacedPartial<int*> namespaced_partial;
  return scalar.kind() + pointer.kind() + reference.kind() +
         pair_primary.kind() + pair_pointer.kind() +
         value_primary.kind() + value_partial.kind() +
         array_primary.kind() + array_partial.kind() +
         namespaced_primary.kind() + namespaced_partial.kind() +
         pointer.refer();
}
