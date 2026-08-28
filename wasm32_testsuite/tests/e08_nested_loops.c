int main(void){
  int t=0;
  for(int i=0;i<20;i++){
    if(i%3==0) continue;
    for(int j=0;j<20;j++){ if(j>i) break; if((i^j)&1) t++; else t+=2; }
    if(t>500) goto out;
  }
out:
  while(t>250) t-=250;
  return t;
}
