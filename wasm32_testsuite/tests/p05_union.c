union U { int i; char b[4]; };
int main(void){ union U u; u.i = 0x01020304; return u.b[0] + u.b[3]; }
