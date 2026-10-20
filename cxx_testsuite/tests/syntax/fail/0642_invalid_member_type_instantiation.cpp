// RUN: -std=c++20
// EXPECT: Invalid type for member 'Child' in instantiation of 'Alias<int>'
// EXPECT: Invalid type for member 'value' in instantiation of 'Field<int>'
// EXPECT: Invalid type for member 'Child' in instantiation of 'Alias<long int>'
// EXPECT: Invalid type for member 'Child' in instantiation of 'Alias<char>'
// EXPECT: Invalid type for member 'Child' in instantiation of 'Alias<short int>'
// EXPECT: Invalid type for member 'value' in instantiation of 'Field<long int>'
// A class template specialization whose member alias or data member type
// does not substitute is ill-formed where it is required to be complete.

template <class T> struct Alias {
  using Child = decltype(missing(T()));
  int y;
};
template <class T> struct Field {
  decltype(missing(T())) value;
  int y;
};
template <class T> struct Derived : Alias<T> {};
template <class T> struct Member {
  Field<T> field;
};

Alias<int> object;
int ReadField(Field<int>* p) { return p->y; }
Derived<long> derived;
// The sizeof operand is first parsed speculatively, which instantiates
// Alias<char>; the diagnostics after it must still be reported.
unsigned long size = sizeof(Alias<char>);
template <class T> int Use() {
  Alias<T> v;
  return v.y;
}
int CallUse() { return Use<short>(); }
Member<long> member;
