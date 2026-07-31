import template_surface;

int main() {
  template_probe::Box box(7);
  if (box.value != 7 || box.add(3L) != 10) {
    return 1;
  }
  if (box.pick(4) != 5 || box.pick(2.0) != 2.5) {
    return 2;
  }
  if (template_probe::Category<long>::value != 0 ||
      template_probe::Category<long*>::value != 1 ||
      template_probe::Category<int>::value != 2) {
    return 3;
  }
  if (template_probe::variable_category<long> != 0 ||
      template_probe::variable_category<long*> != 1) {
    return 4;
  }
  template_probe::NumberSum<2, 3> sum;
  if (sum.get() != 5) {
    return 5;
  }
  if (!template_probe::has_addition<int>()) {
    return 6;
  }
  return 0;
}
