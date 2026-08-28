int counter;
int table[8] = {1,2,3,4,5,6,7,8};
static int scaled = 11 * 3;
struct P { int x, y; };
struct P origin = { 7, 9 };
int bump(int by){ counter += by; return counter; }
int main(void){
  int t = 0;
  for (int i = 0; i < 8; i++) t += table[i];
  bump(4); bump(6);
  return (t + counter + scaled + origin.x + origin.y) % 251;
}
