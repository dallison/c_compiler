struct S { int a:4; int b:12; unsigned c:16; };
struct T { unsigned long long x:40; unsigned long long y:24; };
int main(void){
  struct S s; s.a = -3; s.b = 1000; s.c = 60000;
  struct T t; t.x = 1099511627775ULL; t.y = 12345;
  int r = s.a + s.b + (int)(s.c % 1000);
  r += (int)(t.x % 97) + (int)(t.y % 89);
  s.a = 7; s.b = -2;
  r += s.a * 3 + s.b;
  return (r % 200 + 200) % 200;
}
