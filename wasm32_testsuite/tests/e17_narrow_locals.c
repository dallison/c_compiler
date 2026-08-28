// A variable narrower than the wasm local holding it has to keep dropping
// the bits its old frame slot would have dropped, whether it is written,
// incremented, or arrives as a parameter.

static int taken(signed char sc, unsigned char uc, short s, unsigned short us) {
  // Each parameter is narrower than the i32 it arrives in, and each is
  // written to as well as read, so the trimming has to survive both.
  sc += 100;
  uc += 100;
  s += 30000;
  us += 30000;
  int sum = sc + uc + s + us;

  // One variable whose address is taken keeps a frame, so the two kinds of
  // storage have to work side by side.
  int spilled = sum;
  int* p = &spilled;
  *p += 1;
  return spilled;
}

int main(void) {
  signed char sc = 100;
  unsigned char uc = 250;
  short s = 30000;
  unsigned short us = 60000;

  sc++;
  uc++;
  s++;
  us++;

  unsigned char wrap = 200;
  wrap = wrap * 2;

  short narrowed = (short)70000;
  long long wide = -1;
  unsigned long long uwide = (unsigned long long)wide >> 40;

  int answer = taken(sc, uc, s, us) + wrap + narrowed + (int)uwide;
  return answer & 0xff;
}
