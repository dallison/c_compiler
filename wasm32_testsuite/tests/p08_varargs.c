#include <stdarg.h>
int total(int n, ...){ va_list ap; va_start(ap,n); int t=0; for(int i=0;i<n;i++) t+=va_arg(ap,int); va_end(ap); return t; }
int main(void){ return total(4, 1,2,3,4); }
