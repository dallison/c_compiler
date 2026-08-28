static int a(int x){return x+1;} static int b(int x){return x*2;}
int main(void){ return a(b(a(b(5)))); }
