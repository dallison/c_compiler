int main(void){
  _Bool f = 0, g = 3;
  unsigned long long v = 1ULL << 40;
  int s = 3;
  v >>= s; v |= (unsigned long long)g;
  int t = (int)(v % 1000) + (f?100:1) + (g?2:200);
  signed char sc = -100; sc = (signed char)(sc * 2);
  t += sc;
  return (t % 200 + 200) % 200;
}
