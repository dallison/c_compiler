int f(int x){ switch(x){case 0:return 5;case 1:return 6;case 2:return 7;case 3:return 8;case 4:return 9;case 5:return 10;case 6:return 11;case 7:return 12;case 100:return 13;default:return 14;} }
int main(void){ int t=0; for(int i=0;i<10;i++) t+=f(i); t+=f(100); return t%251; }
