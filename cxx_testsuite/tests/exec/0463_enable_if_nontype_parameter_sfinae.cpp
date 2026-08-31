// RUN: -std=c++11
// EXPECT_EXIT: 0
//
// The pre-C++20 way to constrain an overload is a defaulted non-type template
// parameter whose type is `typename enable_if<C, int>::type`.  Substitution
// must evaluate C per instantiation and discard the candidate when it is false.
//
// The condition is written in one template parameter's type while the parameter
// list is still being read, so the earlier parameters have to be in scope there:
// otherwise `C<T>::value` resolves against C's primary template at parse time,
// every overload answers the same way, and the constraint does nothing.

template <bool B, class T = void>
struct my_enable_if {};
template <class T>
struct my_enable_if<true, T> {
  typedef T type;
};

// Answered by an explicit (full) specialization.
template <class T>
struct is_int {
  static const bool value = false;
};
template <>
struct is_int<int> {
  static const bool value = true;
};

// Answered by a partial specialization.
template <class T>
struct is_ptr {
  static const bool value = false;
};
template <class T>
struct is_ptr<T*> {
  static const bool value = true;
};

// Answered by the primary template alone.
template <class T>
struct always_true {
  static const bool value = true;
};

// Two overloads separated only by the condition in the parameter list.
template <class T, typename my_enable_if<is_int<T>::value, int>::type = 0>
int which(T) {
  return 1;
}
template <class T, typename my_enable_if<!is_int<T>::value, int>::type = 0>
int which(T) {
  return 2;
}

// A partial specialization must drive the condition just as well.
template <class T, typename my_enable_if<is_ptr<T>::value, int>::type = 0>
int pointerness(T) {
  return 1;
}
template <class T, typename my_enable_if<!is_ptr<T>::value, int>::type = 0>
int pointerness(T) {
  return 2;
}

// A single uncontested overload whose condition holds must remain callable.
template <class T, typename my_enable_if<always_true<T>::value, int>::type = 0>
int unconstrained(T) {
  return 3;
}

// The same condition through the return type, which is the other spelling of
// the idiom and must keep working.
template <class T>
typename my_enable_if<is_int<T>::value, int>::type by_return(T) {
  return 1;
}
template <class T>
typename my_enable_if<!is_int<T>::value, int>::type by_return(T) {
  return 2;
}

// A declaration and a separate definition of one constrained template are the
// same template, not two overloads.
template <class T, typename my_enable_if<is_int<T>::value, int>::type = 0>
int declared_then_defined(T);

template <class T, typename my_enable_if<is_int<T>::value, int>::type>
int declared_then_defined(T) {
  return 4;
}

// The condition may also name the enclosing class template's parameters,
// including a pack -- the shape std::variant's converting constructor uses.
// Those have to be substituted when the class is instantiated, before the
// member template's own parameters are renumbered.
template <class T, class... Types>
struct candidate {
  static const bool value = false;
};
template <class... Types>
struct candidate<int, Types...> {
  static const bool value = true;
};

template <class... Types>
struct box {
  int tag;

  box() : tag(0) {}

  template <class T,
            typename my_enable_if<candidate<T, Types...>::value, int>::type = 0>
  box(T) : tag(1) {}
};

struct S {};

int main() {
  if (which(42) != 1) return 1;
  if (which(S()) != 2) return 2;
  if (which('c') != 2) return 3;

  if (pointerness((int*)0) != 1) return 4;
  if (pointerness(S()) != 2) return 5;

  if (unconstrained(42) != 3) return 6;
  if (unconstrained(S()) != 3) return 7;

  if (by_return(42) != 1) return 8;
  if (by_return(S()) != 2) return 9;

  if (declared_then_defined(42) != 4) return 10;

  box<int, char> matching(42);
  if (matching.tag != 1) return 11;
  box<int, char> empty;
  if (empty.tag != 0) return 12;
  return 0;
}
