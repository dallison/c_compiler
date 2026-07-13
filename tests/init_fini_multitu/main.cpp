int lifetime_state;

extern "C" void _Exit(int);
extern "C" int first_anchor();

extern "C" void foreign_preinit() {
  if (lifetime_state == 0) {
    lifetime_state = 1;
  }
}

extern "C" void foreign_fini() {
  _Exit(lifetime_state == 6 ? 0 : 30);
}

int main() {
  if (first_anchor() != 7 || lifetime_state != 3) {
    return lifetime_state + 20;
  }
  lifetime_state = 4;
  return 0;
}
