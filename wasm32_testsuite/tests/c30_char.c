int main(void){ char s[]="hello world"; int t=0; for(int i=0;s[i];i++) if(s[i]=='o') t++; return t*7 + (int)s[0]%64; }
