// RUN: -std=c++20

template <class T>
struct RuntimePartial {
  T value;

  RuntimePartial(T initial) : value(initial) {
  }

  int read(void) {
    return value + 1;
  }
};

template <class T>
struct RuntimePartial<T*> {
  T* value;

  RuntimePartial(T* initial) : value(initial) {
  }

  int read(void) {
    return *value + 20;
  }
};

template <class T, class U>
struct RuntimePair {
  int read(void) {
    return 3;
  }
};

template <class T>
struct RuntimePair<T, int> {
  T value;

  int read(void) {
    return value + 40;
  }
};

int main(void) {
  int pointed = 2;
  RuntimePartial<int> scalar(1);
  RuntimePartial<int*> pointer(&pointed);
  RuntimePair<char, long> primary_pair;
  RuntimePair<int, int> partial_pair;
  partial_pair.value = 5;

  if (scalar.read() != 2) {
    return 1;
  }
  if (pointer.read() != 22) {
    return 2;
  }
  if (primary_pair.read() != 3) {
    return 3;
  }
  if (partial_pair.read() != 45) {
    return 4;
  }
  if (sizeof(RuntimePartial<int*>) == sizeof(RuntimePartial<int>)) {
    return 5;
  }
  return 0;
}
