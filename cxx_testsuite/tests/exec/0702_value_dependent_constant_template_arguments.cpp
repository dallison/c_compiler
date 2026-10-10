// A constant whose initializer depends on template parameters is itself
// value-dependent.  A template-id that uses it as an argument must be formed
// per instantiation, not bound to the primary template.
template <int N>
struct Arr {
  static constexpr int size = N;
  int v[N];
};

template <class T>
struct Traits {
  static constexpr int value = sizeof(T);
};

template <class T>
struct Holder {
  static constexpr int kN = Traits<T>::value;
  Arr<kN> a;
  int Size() const { return Arr<kN>::size; }
  int Local() const {
    constexpr int twice = kN * 2;
    return Arr<twice>::size;
  }
};

template <class T>
int Size() {
  constexpr int n = sizeof(T);
  Arr<n> arr;
  return Arr<n>::size + (int)sizeof(arr) / (int)sizeof(int);
}

template <class T>
int Nested() {
  const int n = Traits<T>::value + 1;
  return Arr<n>::size;
}

int main() {
  if (Size<char>() != 2 || Size<long long>() != 16) return 1;
  if (Nested<short>() != 3) return 2;
  Holder<int> h;
  if (h.Size() != 4 || sizeof(h.a) != 4 * sizeof(int)) return 3;
  if (h.Local() != 8) return 4;
  Holder<char> c;
  if (c.Size() != 1 || c.Local() != 2) return 5;
  return 0;
}
