#include <utility>

template <class... Function>
struct Overload : Function... {
  using Function::operator()...;

  explicit Overload(Function... function)
      : Function(std::move(function))... {}
};

template <class... Function>
Overload(Function...) -> Overload<Function...>;

int main() {
  auto integer = [](int value) { return value + 1; };
  auto floating = [](double value) { return static_cast<int>(value) + 2; };
  Overload overload(integer, floating);
  return overload(1) != 2 || overload(1.0) != 3;
}
