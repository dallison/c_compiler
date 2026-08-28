int main(void){ int t=0; for(int i=0;i<10;i++){ switch(i%4){ case 0: t+=1; break; case 1: t+=10; break; case 2: t+=100; break; default: t+=3; } } return t%256; }
