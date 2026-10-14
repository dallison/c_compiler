// Unary plus on a captureless lambda yields a pointer to function.
#include <type_traits>

using Writer = void (*)(const char*);

static int written = 0;

static void WriteWith(int n, Writer writer) {
  for (int i = 0; i < n; i++) writer("x");
}

int main() {
  WriteWith(3, +[](const char* data) { written += data[0] == 'x'; });
  if (written != 3) return 1;

  auto doubled = +[](int v) { return v * 2; };
  static_assert(std::is_same_v<decltype(doubled), int (*)(int)>);
  if (doubled(21) != 42) return 2;

  auto explicit_return = +[](long a, long b) -> long { return a - b; };
  static_assert(std::is_same_v<decltype(explicit_return), long (*)(long, long)>);
  if (explicit_return(10, 4) != 6) return 3;

  auto nothing = +[] {};
  nothing();

  int (*stored)(int) = +[](int v) { return v + 1; };
  if (stored(1) != 2) return 4;
  return 0;
}
