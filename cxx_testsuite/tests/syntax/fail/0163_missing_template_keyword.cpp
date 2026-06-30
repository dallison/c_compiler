// RUN: -std=c++20 -Werror=missing-template-keyword
// Calling a member template on a type-dependent object without the 'template'
// disambiguator is accepted via a lookahead heuristic, but is non-portable;
// the compiler suggests the 'template' keyword.  Promoted to an error here so
// the diagnostic is observable.
// EXPECT: use 'template' keyword to treat 'get' as a dependent template name

struct Group {
  template <int N>
  int get() {
    return N;
  }
};

template <class T>
int relay(T& g) {
  return g.get<5>();
}
