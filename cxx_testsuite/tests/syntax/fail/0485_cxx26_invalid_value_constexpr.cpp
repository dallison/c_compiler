// RUN: -std=c++26 -fconstexpr-eval=audit
// EXPECT: static_assert expression is not an integer constant expression
// EXPECT: static_assert expression is not an integer constant expression
// EXPECT: static_assert expression is not an integer constant expression

constexpr int erroneous_read() {
  int value;
  return value;
}

constexpr int indeterminate_read() {
  [[indeterminate]] int value;
  return value;
}

constexpr int invalid_unsigned_char_conversion() {
  unsigned char value;
  int converted = value;
  converted = 1;
  return converted;
}

constexpr int indeterminate_dynamic_read() {
  int* value = new int;
  int result = *value;
  delete value;
  return result;
}

static_assert(erroneous_read() == 0);
static_assert(indeterminate_read() == 0);
static_assert(invalid_unsigned_char_conversion() == 1);
static_assert(indeterminate_dynamic_read() == 0);
