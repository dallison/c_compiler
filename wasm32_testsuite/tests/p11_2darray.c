int main(void){ int m[3][4]; int t=0; for(int i=0;i<3;i++) for(int j=0;j<4;j++) m[i][j]=i*4+j; for(int i=0;i<3;i++) for(int j=0;j<4;j++) t+=m[i][j]; return t; }
