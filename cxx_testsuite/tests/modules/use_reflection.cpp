import reflection;

static_assert(reflected_int == ^^int);
static_assert(reflects_int<^^int>());

int main() {
  return 0;
}
