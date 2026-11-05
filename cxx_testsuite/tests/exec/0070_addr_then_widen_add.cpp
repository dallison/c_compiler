// RUN: -std=c++20
// EXPECT_EXIT: 0
// Taking &global must not poison later value uses of that global: widening
// a signed byte into a wider add used to sign-extend the address low byte
// instead of loading the variable (asteroids ship drift with no input).

typedef unsigned _BitInt(16) u16;
typedef _BitInt(8) i8;

extern "C" u16 pos;
extern "C" i8 vel;

extern "C" void bump(i8* p) { *p = (i8)(*p + 1); }

extern "C" u16 step() {
  bump(&vel);
  return (u16)(pos + vel);
}

u16 pos;
i8 vel;

int main() {
  pos = 100;
  vel = 0;
  u16 a = step();
  // vel becomes 1, pos+vel should be 101 — not 100 + lo(&vel).
  if (a != 101) {
    return 1;
  }
  if (vel != 1) {
    return 2;
  }
  u16 b = step();
  if (b != 102) {
    return 3;
  }
  return 0;
}
