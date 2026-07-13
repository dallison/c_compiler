extern int lifetime_state;
extern void note_state(int expected, int next);

__attribute__((constructor)) static void dso_init(void) {
  note_state(1, 2);
}

__attribute__((destructor)) static void dso_fini(void) {
  note_state(4, 5);
}
