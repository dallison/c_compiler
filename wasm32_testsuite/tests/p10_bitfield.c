struct B { unsigned a:3; unsigned b:5; unsigned c:8; };
int main(void){ struct B v; v.a=5; v.b=17; v.c=200; return v.a+v.b+v.c; }
