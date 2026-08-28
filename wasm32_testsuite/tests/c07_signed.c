int main(void){ signed char c=-5; short s=-300; int i=c+s; unsigned char u=(unsigned char)c; return (i+u+1000)%256; }
