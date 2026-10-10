// A class template's `static const int` member initialized by its
// out-of-class definition is a constant per specialization.  It can be a
// template argument or an array bound, inside the class and outside it.
template <int N>
struct Arr {
  static constexpr int size = N;
};

template <class T>
struct Holder {
  static const int kM;
  static const int kTwice;
  int Size() const { return Arr<kM>::size; }
  int Sum() const { return Arr<kM + kTwice>::size; }
  int Bound() const {
    char a[kM];
    return (int)sizeof(a);
  }
};

template <class T>
const int Holder<T>::kM = sizeof(T);
template <class T>
const int Holder<T>::kTwice = Holder<T>::kM * 2;

static_assert(Holder<short>::kM == 2, "");
static_assert(Holder<long long>::kTwice == 16, "");

int main() {
  Holder<short> s;
  if (s.Size() != 2) return 1;
  if (s.Sum() != 6) return 2;
  if (s.Bound() != 2) return 3;
  Holder<int> i;
  if (i.Size() != 4 || i.Bound() != 4) return 4;
  if (Arr<Holder<char>::kM>::size != 1) return 5;
  int a[Holder<long long>::kTwice];
  if (sizeof(a) != 16 * sizeof(int)) return 6;
  return 0;
}
