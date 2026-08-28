int value = 41;
int *pvalue = &value;
int array[4] = {10, 20, 30, 40};
int *third = &array[2];
const char *names[3] = {"aa", "bbb", "cccc"};
int slen(const char *p){ int n=0; while(*p){n++;p++;} return n; }
int main(void){
  int t = *pvalue + *third;
  for (int i = 0; i < 3; i++) t += slen(names[i]);
  return t;
}
