// The 'template' disambiguator keyword in a dependent member access, e.g.
// `g.template onMessage<N>(...)` and `p->template get<N>()`.  The keyword is
// required when the object expression is type-dependent; without it the '<' is
// parsed as a less-than comparison.  Lowering it must behave identically to the
// non-disambiguated form.

struct Group {
  template <int Scale>
  int onMessage(int topic, int slot, int message) {
    return Scale * (topic + slot + message);
  }

  template <int N>
  int get() {
    return N * 10;
  }
};

// Dependent object expression: the 'template' keyword is required here.
template <class T>
int relay(T& g) {
  int topic = 1, slot = 2, message = 3;  // sum == 6
  int a = g.template onMessage<1>(topic, slot, message);  // 6
  int b = g.template onMessage<2>(topic, slot, message);  // 12
  return a + b;                                           // 18
}

// Pointer form via '->template'.
template <class T>
int relay_ptr(T* g) {
  return g->template get<5>();  // 50
}

int main() {
  Group grp;
  if (relay(grp) != 18) {
    return 1;
  }
  if (relay_ptr(&grp) != 50) {
    return 2;
  }
  // The keyword is also accepted on a non-dependent object expression, and must
  // produce the same result as the form without the keyword.
  Group g2;
  if (g2.template onMessage<2>(4, 5, 6) != g2.onMessage<2>(4, 5, 6)) {
    return 3;
  }
  if (g2.template onMessage<2>(4, 5, 6) != 30) {
    return 4;
  }
  return 0;
}
