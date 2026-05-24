//
//  abs.c
//  c_compiler
//
//  Created by David Allison on 6/20/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

// abs/labs/llabs are declared in stdlib.h; no include needed here.

int abs(int j) {
  if (j < 0) {
    return -j;
  }
  return j;
}

long int labs(long int j) {
  if (j < 0) {
    return -j;
  }
  return j;
}

long long int llabs(long long int j) {
  if (j < 0) {
    return -j;
  }
  return j;
}
