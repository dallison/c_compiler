//
//  main.c
//  c_compiler
//
//  Created by David Allison on 10/26/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "dstring.h"
#include "vector.h"
#include <stdio.h>

int main(int argc, const char * argv[]) {
  String* t = StringNew("dave");
  printf("1: %d %zd %zd\n", StringEqual(t, "dave"), t->length, t->capacity);
  
  String t2;
  StringInit(&t2, "dave");
  printf("2: %d\n", StringEqualString(&t2, t));
  
  StringSet(t, "sandra");
  printf("3: %d\n", StringEqual(t, "sandra"));
  
  StringAppend(&t2, " was here");
  printf("4: %d\n", StringEqual(&t2, "dave was here"));
  
  Vector* v1 = VectorNew();
  VectorAppend(v1, t);
  printf("5: %d\n", VectorGet(v1, 0) == (void*)t);
  printf("6: %zd %zd\n", v1->length, v1->capacity);
  
  for (int i = 0; i < 10; i++) {
    VectorAppend(v1, t);
  }
  
  printf("6: %zd %zd\n", v1->length, v1->capacity);
  for (int i = 0; i < 10; i++) {
    printf("7: %d\n", VectorGet(v1, i) == t);
  }
}
