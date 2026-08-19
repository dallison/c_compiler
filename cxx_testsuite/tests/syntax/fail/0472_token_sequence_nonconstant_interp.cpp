// RUN: -std=c++29
// EXPECT: Token interpolation expression is not a constant expression

int runtime_value();

void nonconstant_interp() {
  (void)^{ \(runtime_value()) };
}
