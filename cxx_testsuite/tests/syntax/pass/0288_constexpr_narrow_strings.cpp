// RUN: -std=c++20

constexpr char local_array_value() {
  const char text[] = "format";
  return text[2];
}

constexpr char pointer_value(const char* text) {
  return text[3];
}

static_assert(local_array_value() == 'r');
static_assert(pointer_value("format") == 'm');
