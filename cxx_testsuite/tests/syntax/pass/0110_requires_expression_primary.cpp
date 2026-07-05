// RUN: -std=c++20
struct WithType {
  typedef int type;
};

static_assert(requires { sizeof(int); });
static_assert(requires(int a, int b) { { a + b }; });
static_assert(requires { typename WithType::type; });
static_assert(!requires(int value) { value.missing; });

int main(void) {
  bool ok = requires { sizeof(char); };
  return ok ? 0 : 1;
}
