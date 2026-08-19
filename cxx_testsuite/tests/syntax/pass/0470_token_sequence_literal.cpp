// RUN: -std=c++29

#define TWO_TOKENS foo bar

void token_sequence_literal_parse_test() {
  (void)^{ a + b };
  (void)^{ a += ( };
  (void)^{ int x; };
  (void)^{ TWO_TOKENS ; };
}
