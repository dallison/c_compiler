int lifetime_state;

void note_state(int expected, int next) {
  if (lifetime_state == expected) {
    lifetime_state = next;
  }
}

void exit(int status);

__attribute__((destructor)) static void main_fini(void) {
  note_state(3, 4);
}

int main(void) {
  if (lifetime_state != 2) {
    return lifetime_state + 20;
  }
  lifetime_state = 3;
  exit(100);
}
