typedef enum { RED = 3, GREEN, BLUE = 10 } Color;
typedef struct Node { int v; struct Node *next; } Node;
int main(void){
  Node c = {3, 0}, b = {2, &c}, a = {1, &b};
  int t = 0;
  for (Node *p = &a; p; p = p->next) t += p->v;
  Color k = BLUE;
  switch (k) { case RED: t += 1; break; case BLUE: t += 100; break; default: t += 7; }
  return t;
}
