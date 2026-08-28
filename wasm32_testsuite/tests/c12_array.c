int main(void){ int a[8]; for(int i=0;i<8;i++) a[i]=i*i; int t=0; for(int i=0;i<8;i++) t+=a[i]; return t%256; }
