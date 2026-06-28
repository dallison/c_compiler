// RUN: -std=c++20

unsigned direct_line(void) {
  return __builtin_LINE();
}

unsigned direct_column(void) {
  return __builtin_COLUMN();
}

const char* direct_file(void) {
  return __builtin_FILE();
}

const char* direct_function(void) {
  return __builtin_FUNCTION();
}

const char* direct_pretty_function(void) {
  return __builtin_PRETTY_FUNCTION();
}

unsigned default_line(unsigned line = __builtin_LINE()) {
  return line;
}

