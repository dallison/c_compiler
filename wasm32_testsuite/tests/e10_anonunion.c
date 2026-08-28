struct V { int tag; union { int i; double d; struct { short a, b; }; }; };
int main(void){ struct V v; v.tag=1; v.i=0x00070005; int t=v.tag+v.a+v.b; v.d=2.5; t+=(int)v.d; return t; }
