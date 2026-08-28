int main(void){ int i=0,t=0; loop: if(i>=10) goto done; t+=i; i++; goto loop; done: return t; }
