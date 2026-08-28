const char *g = "hello world";
int slen(const char *p){ int n=0; while(*p){n++;p++;} return n; }
int main(void){
  const char *s = "abcdef";
  int t = slen(s) + slen(g);
  for (int i = 0; g[i]; i++) if (g[i] == 'o') t += 5;
  return t;
}
