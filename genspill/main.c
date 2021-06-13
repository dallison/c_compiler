//
//  main.c
//  genspill
//
//  Created by David Allison on 6/14/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include <stdio.h>

void gen(int n) {
  printf("(");
  const char* s = "";
  for (int i = 0; i < n; i++) {
    printf("%sx%d", s, i);
    s = " + ";
  }
  printf(")");
}

int main(int argc, const char * argv[]) {
  const int N = 30;
  printf("extern void foo(int x);\n");
  printf("void bar() {\n");
  const char* s = "";
  printf("  int ");
  for (int i = 0; i < N; i++) {
    printf("%sx%d = %d", s, i, i);
    s = ", ";
  }
  printf(";\n  foo(");
  gen(N);
  printf(" + ");
  gen(N);
  printf(");\n");
  printf("}\n");
  return 0;
}
