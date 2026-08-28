int sum2(int n){
  int a[n];
  for (int i=0;i<n;i++) a[i]=i*i;
  int t=0;
  { int b[n*2]; for(int i=0;i<n*2;i++) b[i]=i; for(int i=0;i<n*2;i++) t+=b[i]; }
  for (int i=0;i<n;i++) t+=a[i];
  return t;
}
int rec(int n){ if(n<=0) return 0; char pad[n]; for(int i=0;i<n;i++) pad[i]=(char)i; return pad[n-1] + rec(n-1); }
int main(void){ return (sum2(7) + rec(9)) % 251; }
