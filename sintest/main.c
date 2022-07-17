//
//  main.c
//  sintest
//
//  Created by David Allison on 12/30/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdio.h>
#include <math.h>

extern double Sin(double x);
extern double Cos(double x);

int main(int argc, const char * argv[]) {
  for (double a = -M_PI*4; a < M_PI*4; a += 0.01) {
    double x1 = sin(a);
    double x2 = Sin(a);
    double error = x1 - x2;
    if (error > 1e-14) {
      printf("x1: %.20g, x2: %.20g, error: %.20g\n", x1, x2, error);
    }
  }
}
