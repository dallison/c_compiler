int main(void){ int t=0,i=0; loop: if(i>=10) goto done; t+=i; i++; goto loop; done: return t; }
