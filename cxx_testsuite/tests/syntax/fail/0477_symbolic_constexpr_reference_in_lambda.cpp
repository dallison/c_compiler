// RUN: -std=c++26

void invalid_lambda_reference() {
  constexpr int value = 42;
  constexpr const int& reference = value;
  [=] { static_assert(reference == 42); }();
}
