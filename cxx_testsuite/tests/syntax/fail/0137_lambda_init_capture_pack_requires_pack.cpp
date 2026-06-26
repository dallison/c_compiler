// RUN: -std=c++20
// EXPECT: pack expansion requires a function parameter pack

template <class T>
int bad_lambda_init_capture_pack(T value) {
  auto fn = [x = value...] {
    return x;
  };
  return fn();
}
