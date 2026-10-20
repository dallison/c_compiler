// RUN: -std=c++20
// EXPECT_EXIT: 0
// Naming a class template specialization whose member does not substitute
// is fine as long as the specialization is not required to be complete.

template <class T> struct Alias {
  using Child = decltype(missing(T()));
};
template <class T> struct Field {
  decltype(missing(T())) value;
};
template <class T> struct Holder {
  T* p;
};

struct X {};
int missing(X) { return 7; }

template <class T> struct Uses {
  using Child = decltype(missing(T()));
  Child c;
};

using NamedAlias = Alias<int>;
Alias<int>* alias_ptr;
Field<int>* field_ptr;
void TakesAlias(Alias<int>);
Field<int>& FieldRef();
Holder<Field<int>> holder;

int main() {
  Uses<X> u;
  u.c = missing(X());
  NamedAlias* named = alias_ptr;
  return named == nullptr && field_ptr == nullptr && holder.p == nullptr &&
                 u.c == 7
             ? 0
             : 1;
}
