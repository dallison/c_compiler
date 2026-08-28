int main(void){ int a[10]; int*p=a; for(int i=0;i<10;i++) *p++ = i; return (int)(p-a)*4+2; }
