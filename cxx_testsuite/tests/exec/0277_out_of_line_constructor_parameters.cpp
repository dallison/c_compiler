// RUN: -std=c++20
// EXPECT_EXIT: 0

struct Base {
  int value;
  explicit Base(int input) : value(input) {}
};

struct Derived : Base {
  int next;
  explicit Derived(int input);
};

Derived::Derived(int input) : Base(input), next(input + 1) {}

namespace outer {
namespace inner {

struct Holder {
  const char* text;
  explicit Holder(const char* input);
};

Holder::Holder(const char* input) : text(input) {}

}  // namespace inner
}  // namespace outer

int main() {
  Derived derived(41);
  outer::inner::Holder holder("ok");
  return derived.value == 41 && derived.next == 42 &&
                 holder.text[0] == 'o' && holder.text[1] == 'k'
             ? 0
             : 1;
}
