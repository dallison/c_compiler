// RUN: -std=c++20

int global_value = 7;
auto global_auto = 11;

int main(void) {
  auto local = 3;
  auto* ptr = &global_value;
  auto& ref = global_value;
  const auto cv = 5;
  *ptr = local + ref + cv + global_auto;
  return 0;
}
