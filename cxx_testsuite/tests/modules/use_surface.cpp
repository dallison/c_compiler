import surface;

int main() {
  if (mod::k_answer != 42) {
    return 1;
  }
  if (static_cast<int>(mod::Color::Green) != 2) {
    return 2;
  }
  mod::IntAlias alias_value = 11;
  if (alias_value != 11) {
    return 3;
  }
  if (add_one(4) != 5) {
    return 4;
  }
  if (pick(2, 3) != 5) {
    return 5;
  }
  if (pick_def(2) != 3) {
    return 6;
  }
  if (one(4) != 4) {
    return 7;
  }
  if (one(2.5) != 2.5) {
    return 8;
  }
  if (id(5) != 5) {
    return 9;
  }
  if (id<int>(7) != 7) {
    return 10;
  }
  if (mod::inline_ns_value() != 99) {
    return 11;
  }
  mod::Box box{6};
  if (box.value != 6) {
    return 12;
  }
  mod::Holder<int> holder{7};
  if (holder.value != 7) {
    return 13;
  }
  return 0;
}
