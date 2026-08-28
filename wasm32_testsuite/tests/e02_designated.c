struct S { int a; int b[3]; char c; };
struct S g = { .c = 'z', .b = {[2] = 9}, .a = 4 };
int arr[6] = { [4] = 40, [1] = 10 };
int main(void){ int t = g.a + g.b[2] + g.c; for(int i=0;i<6;i++) t+=arr[i]; return t % 251; }
