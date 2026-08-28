static int f(int a,int b,int c,int d,int e){ return a*10000+b*1000+c*100+d*10+e; }
int main(void){ return f(1,2,3,4,5)%256; }
