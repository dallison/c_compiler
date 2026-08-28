struct P { int x, y; };
struct P make(int a, int b){ struct P p; p.x=a; p.y=b; return p; }
int sum(struct P p){ return p.x + p.y; }
int main(void){ struct P p = make(11, 22); struct P q = p; return sum(q); }
