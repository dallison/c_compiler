import constexpr_exceptions;

static_assert(module_exception_value(42) == 42);

int main() {
  return module_exception_value(0);
}
