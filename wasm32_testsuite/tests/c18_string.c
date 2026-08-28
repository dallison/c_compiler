int main(void){ char b[16]; for(int i=0;i<15;i++) b[i]='a'+i; b[15]=0; int t=0; for(int i=0;b[i];i++) t+=b[i]; return t%256; }
