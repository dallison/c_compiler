int main(void){ int x=9; int*p=&x; int**pp=&p; **pp += 33; return x; }
