struct P { int x, y; };
int dot(struct P a, struct P b){ return a.x*b.x + a.y*b.y; }
int main(void){ return dot((struct P){2,3}, (struct P){4,5}) % 200; }
