int main(void){ int t=0; for(int i=0;i<20;i++){ if(i==7) continue; if(i==15) break; t+=i; } return t%256; }
