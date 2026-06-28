#include <source_location>

int string_length(const char* value) {
  int length = 0;
  while (value[length] != '\0') {
    length++;
  }
  return length;
}

bool string_equal(const char* left, const char* right) {
  int index = 0;
  while (left[index] != '\0' || right[index] != '\0') {
    if (left[index] != right[index]) {
      return false;
    }
    index++;
  }
  return true;
}

bool string_ends_with(const char* value, const char* suffix) {
  int value_length = string_length(value);
  int suffix_length = string_length(suffix);
  if (suffix_length > value_length) {
    return false;
  }
  int start = value_length - suffix_length;
  for (int i = 0; i < suffix_length; i++) {
    if (value[start + i] != suffix[i]) {
      return false;
    }
  }
  return true;
}

std::source_location forward_location(
    std::source_location loc = std::source_location::current()) {
  return loc;
}

int main(void) {
  unsigned int expected_line = __LINE__ + 1;
  unsigned int direct_line = __builtin_LINE();
  if (direct_line != expected_line) {
    return 1;
  }
  if (__builtin_COLUMN() == 0) {
    return 2;
  }
  if (!string_ends_with(__builtin_FILE(),
                        "0072_standard_source_location_header.cpp")) {
    return 3;
  }
  if (!string_equal(__builtin_FUNCTION(), "main")) {
    return 4;
  }
  if (!string_equal(__builtin_PRETTY_FUNCTION(), "int main(void)")) {
    return 5;
  }

  expected_line = __LINE__ + 1;
  std::source_location loc = std::source_location::current();
  if (loc.line() != expected_line) {
    return 6;
  }
  if (loc.column() == 0) {
    return 7;
  }
  if (!string_ends_with(loc.file_name(),
                        "0072_standard_source_location_header.cpp")) {
    return 8;
  }
  if (!string_equal(loc.function_name(), "int main(void)")) {
    return 9;
  }

  expected_line = __LINE__ + 1;
  std::source_location forwarded = forward_location();
  if (forwarded.line() != expected_line) {
    return 10;
  }
  if (!string_equal(forwarded.function_name(), "int main(void)")) {
    return 11;
  }

  std::source_location empty;
  if (empty.line() != 0 || !string_equal(empty.file_name(), "")) {
    return 12;
  }
  return 0;
}

