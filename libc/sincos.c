//
//  sincos.c
//  c_compiler
//
//  Created by David Allison on 12/26/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <math.h>
#include <limits.h>


static const double
twoopi = 0.63661977236758134308,
p0 = 0.1357884097877375669092680e8,
p1 = -0.4942908100902844161158627e7,
p2 = 0.4401030535375266501944918e6,
p3 = -0.1384727249982452873054457e5,
p4 = 0.1459688406665768722226959e3,
q0 = 0.8644558652922534429915149e7,
q1 = 0.4081792252343299749395779e6,
q2 = 0.9463096101538208180571257e4,
q3 = 0.1326534908786136358911494e3;


static double __sin(double x, int quad) {
  if (x < 0) {
    x = -x;
    quad += 2;
  }
  x *= twoopi;
  double y;
  if (x > LLONG_MAX-1) {
    double e;
    double f;
    y = modf(x, &e);
    e += quad;
    modf(0.25*e, &f);
    quad = (int)e - (int)(4.0*f);
  } else {
    long long k = x;
    y = x - (double)k;
    quad = (quad + k) & 3;
  }
  if ((quad & 1) != 0) {
    y = 1 - y;
  }
  if (quad > 1) {
    y = -y;
  }
  double y2 = y * y;
  double t1 = ((((p4*y2+p3)*y2+p2)*y2+p1)*y2+p0)*y;
  double t2 = ((((y2+q3)*y2+q2)*y2+q1)*y2+q0);
  return t1/t2;
}

double sin(double x) {
  return __sin(x, 0);
}

double cos(double x) {
  if (x < 0) {
    x = -x;
  }
  return __sin(x, 1);
}
