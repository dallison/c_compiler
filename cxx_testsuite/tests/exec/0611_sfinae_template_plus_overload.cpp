// RUN: -std=c++17
// EXPECT_EXIT: 0

// FormatConvertImpl-shaped: an overload set whose first candidate is a
// function template with a SFINAE trailing-return, plus a non-template
// overload for unsigned.  decltype of a concrete call must pick the
// non-template, not treat the template name as a dependent callee.

void hook();

template <typename T>
auto convert(const T& v) -> decltype(hook(v)) {
  return hook(v);
}

struct Result {
  int value;
};

Result convert(unsigned) { return Result{7}; }

template <typename T>
T& dummy();

template <typename T>
constexpr int conv_of() {
  using R = decltype(convert(dummy<const T&>()));
  return (int)sizeof(R);
}

int main() {
  return conv_of<unsigned>() == (int)sizeof(Result) ? 0 : 1;
}
