int main(void){ int t=0,i=0; if(i) goto b; a: t+=1; i++; if(i<5) goto b; return t; b: t+=10; i++; if(i<5) goto a; return t; }
