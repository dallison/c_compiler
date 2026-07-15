import reachability;

int main() {
  if (add_hidden(4) != 7) {
    return 1;
  }
  if (forwarded_template(5) != 8) {
    return 2;
  }
  auto hidden = make_hidden();
  if (hidden.value != 7) {
    return 3;
  }
  if (ordinary_uses_hidden() != 99) {
    return 4;
  }
  short value = 1;
  return visible_overload(value) == 2 ? 0 : 5;
}
