extern int lifetime_state;

struct Second {
  Second() {
    if (lifetime_state == 1) {
      lifetime_state = 2;
    }
  }

  ~Second() {
    if (lifetime_state == 5) {
      lifetime_state = 6;
    }
  }
};

Second second;
