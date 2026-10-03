// C99 signed division truncates toward zero and the remainder takes the sign
// of the dividend, for every combination of operand signs and for 16-, 32- and
// 64-bit operands.  `x / 8` also covers the power-of-two lowering.
volatile int dividends[] = {17, -17, 7, -7, 0, 16};
volatile int divisors[] = {8, -8, 2, -2, 3, -3, 1, -1};

static const signed char quotients[6][8] = {
    {2, -2, 8, -8, 5, -5, 17, -17},  {-2, 2, -8, 8, -5, 5, -17, 17},
    {0, 0, 3, -3, 2, -2, 7, -7},     {0, 0, -3, 3, -2, 2, -7, 7},
    {0, 0, 0, 0, 0, 0, 0, 0},        {2, -2, 8, -8, 5, -5, 16, -16},
};
static const signed char remainders[6][8] = {
    {1, 1, 1, 1, 2, 2, 0, 0},         {-1, -1, -1, -1, -2, -2, 0, 0},
    {7, 7, 1, 1, 1, 1, 0, 0},         {-7, -7, -1, -1, -1, -1, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},         {0, 0, 0, 0, 1, 1, 0, 0},
};

int main(void) {
  for (int i = 0; i < 6; i++) {
    int a = dividends[i];
    if (a / 8 != quotients[i][0] || a % 8 != remainders[i][0]) {
      return 200 + i;
    }
    for (int j = 0; j < 8; j++) {
      int b = divisors[j];
      long la = a;
      long lb = b;
      long long lla = a;
      long long llb = b;
      if (a / b != quotients[i][j] || la / lb != quotients[i][j] ||
          lla / llb != quotients[i][j]) {
        return 1 + i * 8 + j;
      }
      if (a % b != remainders[i][j] || la % lb != remainders[i][j] ||
          lla % llb != remainders[i][j]) {
        return 101 + i * 8 + j;
      }
    }
  }
  return 0;
}
