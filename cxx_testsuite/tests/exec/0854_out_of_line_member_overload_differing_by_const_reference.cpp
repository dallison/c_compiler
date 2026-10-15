// Out-of-line definitions of class template members overloaded on `const X&`
// versus `X&` each define their own declaration (protobuf's
// MapTypeHandler<TYPE_STRING, Type>::DeleteNoArena in a partial
// specialization).  The non-const one was attached to the const declaration.
struct StrPtr {
  int* p;
  void Destroy() { *p += 10; }
};

template <int F, typename T>
struct Traits;
template <typename T>
struct Traits<9, T> {
  typedef StrPtr TypeOnMemory;
};

template <int F, typename T>
class Handler;

template <typename T>
class Handler<9, T> {
 public:
  typedef typename Traits<9, T>::TypeOnMemory TypeOnMemory;
  static inline void DeleteNoArena(const TypeOnMemory& x);
  static void DeleteNoArena(TypeOnMemory& value);
};

template <typename T>
void Handler<9, T>::DeleteNoArena(TypeOnMemory& value) {
  value.Destroy();
}

template <typename T>
class Both {
 public:
  void Touch(const StrPtr& x) const;
  void Touch(StrPtr& x) const;
};

template <typename T>
void Both<T>::Touch(StrPtr& x) const {
  x.Destroy();
}

template <typename T>
void Both<T>::Touch(const StrPtr& x) const {
  *x.p += 100;
}

int main() {
  int n = 1;
  StrPtr s{&n};
  Handler<9, long>::DeleteNoArena(s);
  if (n != 11) {
    return 1;
  }
  Both<int> both;
  both.Touch(s);
  const StrPtr& cs = s;
  both.Touch(cs);
  return n == 121 ? 0 : 2;
}
