int main(void){ long long x=1; for(int i=0;i<40;i++) x*=2; return (int)((x>>33)&0xFF); }
