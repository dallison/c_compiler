int add(int a,int b){return a+b;}
int mul(int a,int b){return a*b;}
int main(void){ int (*f)(int,int) = add; int t=f(3,4); f=mul; return t+f(5,6); }
