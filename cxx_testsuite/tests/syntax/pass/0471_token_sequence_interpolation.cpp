// RUN: -std=c++29

template <typename T>
void sink(T);

void token_sequence_interpolation_parse_test() {
  (void)^{ \(1) + \(2) };
  (void)^{ \id("prefix", 1) };
  (void)^{ \tokens(^{ a ; }) };
}
