extern int lifetime_state;

extern "C" int first_anchor() {
  return 7;
}

struct First {
  First() {
    if (lifetime_state == 2) {
      lifetime_state = 3;
    }
  }

  ~First() {
    if (lifetime_state == 4) {
      lifetime_state = 5;
    }
  }
};

First first;
