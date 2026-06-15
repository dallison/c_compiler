int global_value = 4;
auto global_auto = 9;

int main(void) {
  auto local = 3;
  auto* ptr = &global_value;
  auto& ref = global_value;
  const auto cv = 5;

  ref = local + global_auto;
  if (global_value != 12) {
    return 1;
  }
  *ptr = ref + cv;
  if (global_value != 17) {
    return 2;
  }
  return 0;
}
