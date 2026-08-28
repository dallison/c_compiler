#include <stdarg.h>
double average(int n, ...){
  va_list ap; va_start(ap, n);
  double t = 0;
  for (int i = 0; i < n; i++) t += va_arg(ap, double);
  va_end(ap);
  return t / n;
}
long long mix(const char *fmt, ...){
  va_list ap, copy;
  va_start(ap, fmt);
  va_copy(copy, ap);
  long long t = 0;
  for (const char *p = fmt; *p; p++) {
    if (*p == 'i') t += va_arg(ap, int);
    else if (*p == 'l') t += va_arg(ap, long long);
    else if (*p == 'p') t += *va_arg(ap, int *);
  }
  t += va_arg(copy, int);     /* the first one again */
  va_end(copy);
  va_end(ap);
  return t;
}
int outer(int a, ...){ va_list ap; va_start(ap,a); int x = va_arg(ap,int); va_end(ap); return x + (int)average(2, 4.0, 6.0); }
int main(void){
  int q = 17;
  long long t = mix("ilp", 3, 40000000000LL, &q);
  t += (long long)average(3, 1.5, 2.5, 6.0);
  t += outer(1, 9);
  return (int)(t % 211);
}
