struct S { int a; char b; short c; };
int main(void){ struct S s[4]; for(int i=0;i<4;i++){ s[i].a=i; s[i].b=(char)(i*2); s[i].c=(short)(i*3);} int t=0; for(int i=0;i<4;i++) t+=s[i].a+s[i].b+s[i].c; return t; }
