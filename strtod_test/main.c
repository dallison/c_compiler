//
//  main.c
//  strtod_test
//
//  Created by David Allison on 12/1/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdio.h>

extern double Strtod(const char* s, char** end);

int main(int argc, const char * argv[]) {
  double x = Strtod("-3.14159265", NULL);
  printf("%g\n", x);
}
