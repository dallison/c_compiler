int n = 6;
int use(char *p, int k){ int t=0; for(int i=0;i<k;i++) t+=p[i]; return t; }
int fill(void){ char a[n]; for (int i=0;i<n;i++) a[i]=(char)(i+1); return use(a, n); }
int main(void){ int t=0; for (int i=0;i<3;i++) t += fill(); return t; }
