// RUN: -std=c++20

[[maybe_unused]] int global_with_attribute = 1;

[[nodiscard]] int compute_value(void) {
  return 3;
}

[[noreturn]] void declared_noreturn(void);

struct [[gnu::packed]] PackedSyntax {
  char c;
  int i;
};

struct MemberAttributes {
  [[maybe_unused]] int field;
  [[nodiscard]] int method(void) { return field; }
};

enum [[deprecated("old enum")]] OldEnum {
  kOldEnumValue
};

enum class [[maybe_unused]] ScopedEnum : int {
  kValue = 1
};

void use_statement_attributes(int value) {
  switch (value) {
    case 0:
      value = 1;
      [[fallthrough]];
    default:
      break;
  }

  if (value) [[likely]] {
    value = compute_value();
  } else [[unlikely]] {
    value = 0;
  }

  [[maybe_unused]] int local = value;
}

struct UsingNamespaceAttribute {
  [[using gnu: aligned(8)]] int aligned_field;
};
