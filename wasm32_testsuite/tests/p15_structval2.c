struct Big { int a[6]; double d; char tag; };
struct Small { short x, y; };
struct Big build(int seed){
  struct Big b;
  for (int i = 0; i < 6; i++) b.a[i] = seed + i;
  b.d = seed * 1.5;
  b.tag = (char)(seed & 0x7f);
  return b;
}
int consume(struct Big b, struct Small s){
  b.a[0] = 999;              /* must not touch the caller's copy */
  return b.a[1] + b.a[5] + (int)b.d + b.tag + s.x + s.y;
}
struct Small pair(short a, short b){ struct Small s; s.x=a; s.y=b; return s; }
struct Big global;
int main(void){
  struct Big b = build(4);
  struct Small s = pair(3, 8);
  int t = consume(b, s);
  t += b.a[0];               /* still 4 if the copy was honoured */
  global = build(10);
  t += global.a[5] + (int)global.d;
  struct Big c = global;
  t += c.a[2];
  return t % 251;
}
