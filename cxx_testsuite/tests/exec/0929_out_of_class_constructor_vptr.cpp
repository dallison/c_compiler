// Constructors of class templates defined outside the class.  Their bodies
// are cloned from the pattern, so the vtable pointers they store must be the
// instantiation's vtables, not the pattern's (gtest's Matcher<T>::Matcher(T)).
// Each instantiation's virtuals differ, so sharing one vtable is visible.
#include <stdio.h>

template <class T> struct Kind { static const int value = 2; };
template <> struct Kind<long> { static const int value = 40; };
template <> struct Kind<short> { static const int value = 50; };

template <class T> struct Base {
  Base() {}
  virtual ~Base() {}
  virtual int Describe() const { return 1; }
};

struct Other {
  virtual ~Other() {}
  virtual int Tag() const { return 10; }
};

template <class T> struct Mat : Base<T> {
  Mat() {}
  Mat(T value);
  template <class U> Mat(U* p, int extra);
  int Describe() const override { return Kind<T>::value + seen; }
  int seen = 0;
};

template <class T> Mat<T>::Mat(T value) : seen(value) {}

template <class T>
template <class U>
Mat<T>::Mat(U* p, int extra) : seen(*p + extra) {
  seen = Describe();
}

template <class T> struct Both : Base<T>, Other {
  explicit Both(T v);
  int Describe() const override { return 100 + v_ + Kind<T>::value; }
  int Tag() const override { return 200 + v_; }
  int during = 0;
  T v_;
};

template <class T> Both<T>::Both(T v) : v_(v) { during = Describe() + Tag(); }

int main() {
  Mat<const int&> m(5);
  Base<const int&>* b = &m;
  if (b->Describe() != 7) return 1;

  int x = 3;
  Mat<long> n(&x, 4);
  Base<long>* bn = &n;
  if (bn->Describe() != 87) return 2;

  Both<int> both(1);
  Base<int>* bb = &both;
  Other* ob = &both;
  if (bb->Describe() != 103) return 3;
  if (ob->Tag() != 201) return 4;
  if (both.during != 304) return 5;

  Base<short>* heap = new Both<short>(2);
  if (heap->Describe() != 152) return 6;
  delete heap;
  printf("ok\n");
  return 0;
}
