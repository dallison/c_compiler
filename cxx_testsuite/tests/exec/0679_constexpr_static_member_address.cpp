// RUN: -std=c++17
// EXPECT_EXIT: 0
// A constexpr constructor that stores the address of one of the object's own
// members constant-initializes a static with that member's address.

struct Inner {
  const int* p;
  int v = 4;
  constexpr Inner() : p(&v) {}
};

struct First {
  int a;
  const int* p;
  constexpr First() : a(1), p(&a) {}
};

struct Array {
  int values[3] = {5, 6, 7};
  const int* middle;
  constexpr Array() : middle(&values[1]) {}
};

const Inner* GetInner() {
  static constexpr Inner inner;
  return &inner;
}

const First* GetFirst() {
  static constexpr First first;
  return &first;
}

const Array* GetArray() {
  static constexpr Array array;
  return &array;
}

int main() {
  const Inner* inner = GetInner();
  if (inner->p != &inner->v || *inner->p != 4) {
    return 1;
  }
  const First* first = GetFirst();
  if (first->p != &first->a || *first->p != 1) {
    return 2;
  }
  const Array* array = GetArray();
  if (array->middle != &array->values[1] || *array->middle != 6) {
    return 3;
  }
  return 0;
}
