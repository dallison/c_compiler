int main(void){ int t=0; for(int i=-3;i<=3;i++){ if(i<0)t+=1; if(i<=0)t+=2; if(i>0)t+=4; if(i>=0)t+=8; if(i==0)t+=16; if(i!=0)t+=32; } return t%256; }
