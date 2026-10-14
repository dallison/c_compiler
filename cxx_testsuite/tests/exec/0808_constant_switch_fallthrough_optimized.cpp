// RUN: -std=c++17 -O2
// EXPECT_EXIT: 0

// With optimization a switch on a constant branches straight to the selected
// label, but statements under later labels are still reached by falling
// through.  Abseil's `switch (0) case 0: default: stmt;` logging idiom puts
// the whole statement under the label after the selected one.

#define LOG_LIKE(counter) \
  switch (0)              \
  case 0:                 \
  default:                \
    counter += 100

int Idiom(int start) {
  int counter = start;
  LOG_LIKE(counter);
  return counter;
}

int Fallthrough(int start) {
  int y = start;
  switch (1) {
    case 0:
      y += 1000;
    case 1:
      y += 1;
    case 2:
      y += 10;
      [[fallthrough]];
    default:
      y += 100;
      break;
    case 3:
      y += 5000;
  }
  return y;
}

int NoMatchUsesDefault(int start) {
  int y = start;
  switch (7) {
    case 1:
      y += 1;
      break;
    default:
      y += 2;
    case 2:
      y += 20;
  }
  return y;
}

int main(int argc, char**) {
  int start = argc > 100 ? 1 : 0;
  if (Idiom(start) != 100) return 1;
  if (Fallthrough(start) != 111) return 2;
  if (NoMatchUsesDefault(start) != 22) return 3;
  return 0;
}
