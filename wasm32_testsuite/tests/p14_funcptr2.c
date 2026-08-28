int add(int a,int b){return a+b;}
int sub(int a,int b){return a-b;}
int apply(int (*f)(int,int), int a, int b){ return f(a,b); }
int (*table[2])(int,int) = { add, sub };
void bump(int *p){ *p += 5; }
int main(void){
  int t = apply(add, 10, 3) + apply(sub, 10, 3);
  for (int i = 0; i < 2; i++) t += table[i](20, 4);
  int (*f)(int,int) = add;
  if (f == add) t += 1;
  if (f != sub) t += 2;
  void (*v)(int*) = bump;
  v(&t);
  return t % 200;
}
