#include <stdarg.h>
int total(int n, ...){ va_list ap; va_start(ap,n); long long t=0; for(int i=0;i<n;i++) t+=va_arg(ap,int); va_end(ap); return (int)(t%1000); }
int main(void){ int t=0; for(int i=0;i<5000;i++) t=(t+total(6,i,i+1,i+2,i+3,i+4,i+5))%251; return t; }
