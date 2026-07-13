extern int lifetime_state;
extern void note_state(int expected, int next);
void _Exit(int status);

__attribute__((constructor)) static void dep_init(void) {
  note_state(0, 1);
}

__attribute__((destructor)) static void dep_fini(void) {
  note_state(5, 6);
  _Exit(lifetime_state == 6 ? 0 : 41);
}
