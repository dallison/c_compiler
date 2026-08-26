#include <stdio.h>

#define ONE else if (0) {}
#define TEN ONE ONE ONE ONE ONE ONE ONE ONE ONE ONE
#define HUNDRED TEN TEN TEN TEN TEN TEN TEN TEN TEN TEN
#define THOUSAND \
  HUNDRED HUNDRED HUNDRED HUNDRED HUNDRED \
  HUNDRED HUNDRED HUNDRED HUNDRED HUNDRED

int main(void) {
  int reached_final_else = 0;
  if (0) {}
  THOUSAND THOUSAND THOUSAND THOUSAND THOUSAND THOUSAND
  THOUSAND THOUSAND THOUSAND THOUSAND THOUSAND
  else reached_final_else = 1;
  if (reached_final_else != 1) return 1;
  puts("ok");
  return 0;
}
