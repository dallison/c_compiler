// RUN: -std=c++20
namespace outer {
inline namespace {
int inline_anon_value = 1;
}
}

namespace {
int ordinary_anon_value = 2;
}

namespace outer {
namespace {
int reopened_anon_value = 3;
}
}

void use_qualified(void) {
  (void)outer::inline_anon_value;
  (void)ordinary_anon_value;
  (void)outer::reopened_anon_value;
}

void use_unqualified(void) {
  using namespace outer;
  (void)inline_anon_value;
  (void)reopened_anon_value;
}

int main(void) {
  use_qualified();
  use_unqualified();
  return outer::inline_anon_value + ordinary_anon_value +
         outer::reopened_anon_value;
}
