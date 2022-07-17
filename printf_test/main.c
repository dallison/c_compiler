//
//  main.c
//  printf_test
//
//  Created by David Allison on 7/10/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdio.h>
#include <stdarg.h>

extern int __printf(const char* s, ...);

int main(int argc, const char * argv[]) {
  float f = 56.78;
  printf("%f\n", f);
  __printf("%f\n", f);
}
