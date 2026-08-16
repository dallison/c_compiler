import reflection_extended;

static_assert(reflected_int == ^^int);

constexpr auto imported_ns = ^^alias_ns;
static_assert([:imported_ns:]::value == 7);

int main() {
  secondary<int> s;
  (void)s.item;
  return 0;
}
