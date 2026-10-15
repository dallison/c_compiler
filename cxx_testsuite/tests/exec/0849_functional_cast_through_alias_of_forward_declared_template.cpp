// An alias formed while its class template is only forward-declared names a
// deferred specialization.  `ValueView(4)` after the definition constructs
// that specialization (protobuf's Printer::ValueView).
struct Printer {
  template <bool owned>
  struct ValueImpl;
  using ValueView = ValueImpl<false>;
  using Value = ValueImpl<true>;
};

template <bool owned>
struct Printer::ValueImpl {
  ValueImpl(int v) : value(v + (owned ? 10 : 0)) {}
  int value = 0;
};

template <bool owned>
struct Box;
using SmallBox = Box<false>;
template <bool owned>
struct Box {
  Box(int a, int b) : sum(a + b) {}
  int sum;
};

int main() {
  if (Printer::ValueView(4).value != 4 || Printer::Value(4).value != 14) {
    return 1;
  }
  return SmallBox(2, 3).sum == 5 ? 0 : 2;
}
