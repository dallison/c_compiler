// `Template{args}` deduces the template's arguments from its deduction guides,
// including for aggregates whose bases come from the template parameters.
template <typename... Ts>
struct Overloaded : Ts... {
  using Ts::operator()...;
};
template <typename... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

template <typename T>
struct One : T {
  using T::operator();
};
template <typename T>
One(T) -> One<T>;

template <typename T, typename U>
struct Pair {
  T a;
  U b;
};
template <typename T, typename U>
Pair(T, U) -> Pair<T, U>;

template <typename T>
struct Box {
  T v;
  Box<T> Twice() const { return Box{v * 2}; }
};
template <typename T>
Box(T) -> Box<T>;

struct Inc {
  int operator()(int v) const { return v + 1; }
};
struct Forty {
  int operator()(double) const { return 40; }
};

int main() {
  auto both = Overloaded{Inc{}, Forty{}};
  if (both(1) != 2 || both(1.5) != 40) return 1;
  auto one = One{Inc{}};
  if (one(4) != 5) return 2;
  auto p = Pair{1, 2.5};
  if (p.a != 1 || p.b != 2.5 || sizeof(p.b) != sizeof(double)) return 3;
  if (Pair{7, 'c'}.b != 'c') return 4;
  if (Box{3}.Twice().v != 6) return 5;
  auto explicit_args = Pair<int, int>{4, 5};
  if (explicit_args.a + explicit_args.b != 9) return 6;
  return 0;
}
