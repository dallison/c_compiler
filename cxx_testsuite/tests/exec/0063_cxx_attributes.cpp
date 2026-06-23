// RUN: -std=c++20

struct [[gnu::packed]] Packed {
  char c;
  int i;
};

struct Natural {
  char c;
  int i;
};

[[nodiscard]] int nodiscard_value(void) {
  return 5;
}

[[gnu::aligned(16)]] int aligned_global = 7;

int main(void) {
  if (sizeof(Packed) != sizeof(char) + sizeof(int)) {
    return 1;
  }
  if (sizeof(Natural) <= sizeof(Packed)) {
    return 2;
  }

  Packed packed;
  packed.c = 1;
  packed.i = 0x10203;
  if (packed.c != 1 || packed.i != 0x10203) {
    return 3;
  }

  [[maybe_unused]] int ignored_local = nodiscard_value();
  if (aligned_global != 7) {
    return 4;
  }

  return 0;
}
