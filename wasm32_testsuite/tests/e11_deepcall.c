struct R { int a,b,c,d,e,f,g,h; };
struct R g1(int s){ struct R r={s,s+1,s+2,s+3,s+4,s+5,s+6,s+7}; return r; }
struct R g2(struct R x){ x.a*=2; return x; }
int g3(struct R x, int k){ return x.a+x.h+k; }
int main(void){ return g3(g2(g1(3)), 5) % 251; }
