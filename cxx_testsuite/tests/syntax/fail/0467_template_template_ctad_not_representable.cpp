// RUN: -std=c++20

template <typename... Ts>
struct Variadic {
  explicit Variadic(Ts...) {}
};

template <template <typename> typename Container>
void invalid_deduction() {
  Container object(1, 2);
}

int main() {
  invalid_deduction<Variadic>();
}
