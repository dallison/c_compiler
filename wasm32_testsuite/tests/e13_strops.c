static char buf[64];
int cp(char *d, const char *s){ int n=0; while((*d++=*s++)) n++; return n; }
int cmp(const char*a,const char*b){ while(*a&&*a==*b){a++;b++;} return *(const unsigned char*)a-*(const unsigned char*)b; }
int main(void){ int n=cp(buf,"the quick brown fox"); int t=n+(cmp(buf,"the quick brown fox")==0?10:0)+(cmp(buf,"zzz")<0?20:0); return t; }
