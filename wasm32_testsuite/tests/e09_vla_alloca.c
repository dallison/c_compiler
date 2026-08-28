int sum(int n){ int a[n]; for(int i=0;i<n;i++) a[i]=i*i; int t=0; for(int i=0;i<n;i++) t+=a[i]; return t; }
int main(void){ return sum(9) % 251; }
