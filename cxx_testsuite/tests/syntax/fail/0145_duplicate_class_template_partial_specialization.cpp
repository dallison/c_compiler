// RUN: -std=c++20
// EXPECT: Duplicate class template partial specialization

template <class T>
struct DuplicatePartial {
  int value;
};

template <class T>
struct DuplicatePartial<T*> {
  T* value;
};

template <class U>
struct DuplicatePartial<U*> {
  U* value;
};
