int main(void){ int a=5,b=7,t=0; t = (a>b) ? a : (b>3 ? b*2 : 0); t += (a++, b--, a+b); do { t++; } while (t % 5); return t % 200; }
