struct E { int k; char pad[7]; };
struct E table[5];
int main(void){
  for (int i=0;i<5;i++) table[i].k = i*i;
  struct E *p = table + 4, *q = &table[1];
  int t = (int)(p - q) + p->k + q->k;
  int *ip = &table[2].k; ip += 0;
  t += *ip;
  char *cp = (char*)table; t += cp[0];
  return t;
}
