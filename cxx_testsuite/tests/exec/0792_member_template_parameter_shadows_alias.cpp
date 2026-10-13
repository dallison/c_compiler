#include <string>

struct Printer {
  template <bool Owned>
  struct ValueImpl;

  using Value = ValueImpl<true>;
};

template <bool Owned>
struct Printer::ValueImpl {
  template <class Value>
  ValueImpl(Value&&) {}
};

int main() {
  const std::string text = "value";
  Printer::ValueImpl<false> value(text);
  (void)value;
}
