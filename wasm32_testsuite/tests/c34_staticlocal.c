int next(void){ static int n = 100; n += 3; return n; }
int main(void){ next(); next(); return next() % 200; }
