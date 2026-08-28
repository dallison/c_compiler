int depth(int n){ if(n<=0) return 0; int pad[8]; for(int i=0;i<8;i++) pad[i]=n+i; return pad[0]%3 + depth(n-1); }
int main(void){ return depth(2000) % 251; }
