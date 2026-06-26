// RUN: -std=c++20
// EXPECT: pack expansion requires a function parameter pack

int accepts_one(int value);

template <class T>
int bad_forward(T value) {
  return accepts_one(value...);
}
