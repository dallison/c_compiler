int main(void){ int t=0; for(int i=0;i<8;i++) for(int j=0;j<8;j++) if((i^j)&1) t++; return t%256; }
