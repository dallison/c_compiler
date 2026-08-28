static const char *const names[] = {"alpha","beta","gamma"};
static const int weights[] = {1,2,3};
int slen(const char*p){int n=0;while(*p){n++;p++;}return n;}
int main(void){ int t=0; for(int i=0;i<3;i++) t += slen(names[i])*weights[i]; return t; }
