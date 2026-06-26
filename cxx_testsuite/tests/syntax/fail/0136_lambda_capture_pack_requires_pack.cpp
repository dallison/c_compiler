// RUN: -std=c++20
// EXPECT: lambda capture pack expansion requires a function parameter pack

template <class T>
int bad_lambda_capture_pack(T value) {
  auto fn = [value...] {
    return value;
  };
  return fn();
}
