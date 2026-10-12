// RUN: -std=c++17
// EXPECT_EXIT: 0

namespace example {

struct Outer {
  struct Inner {
    explicit Inner(int value);
    int value;
  };
};

}  // namespace example

example::Outer::Inner::
    Inner(int value) : value(value) {}

int main() {
  example::Outer::Inner value(32);
  return value.value == 32 ? 0 : 1;
}
