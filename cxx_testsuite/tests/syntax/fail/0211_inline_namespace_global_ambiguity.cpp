// RUN: -std=c++20
int value = 1;

inline namespace V {
int value = 2;
}

void use_ambiguous_value(void) {
  (void)value;
}
