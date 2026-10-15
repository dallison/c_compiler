// Inside a nested class of a class template, the enclosing template's
// injected-class-name with explicit arguments names that specialization
// (`Spy<void>{}` in `Spy<T>::Combiner`, as abseil's hash test spy does).
template <typename T>
struct Spy {
  int value = 0;

  struct Combiner {
    Spy<void> Make(int v) const {
      Spy<void> s = Spy<void>{};
      s.value = v;
      return s;
    }
    Spy<int> MakeParenthesized() const { return Spy<int>(); }
  };

  struct Outer {
    struct Inner {
      int Get() const { return Spy<long>{}.value + 2; }
    };
  };
};

int main() {
  Spy<char>::Combiner combiner;
  if (combiner.Make(5).value != 5) return 1;
  if (combiner.MakeParenthesized().value != 0) return 2;
  if (Spy<short>::Outer::Inner().Get() != 2) return 3;
  return 0;
}
