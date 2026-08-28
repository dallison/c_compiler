int f(int x){ switch(x){ case 1: case 2: return 10; case 3: return 20; default: return 30; } }
int main(void){ return f(1)+f(2)+f(3)+f(9); }
