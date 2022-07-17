//
//  exit.c
//  c_compiler
//
//  Created by David Allison on 6/17/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include <stdlib.h>

static void (*atexit_funcs[32])(void);
static unsigned char numfuncs;

int atexit(void (*p)(void)) {
  if (numfuncs > 31) {
    return -1;
  }
  unsigned char n = numfuncs++;
  atexit_funcs[n] = p;
  return 0;
}

// If atexit is called from an atexit func it must be called after all
// currently registered atexit funcs.
void exit(int status) {
  while (numfuncs > 0) {
    void (*funcs[32])(void);
    memcpy(funcs, atexit_funcs, sizeof(funcs));
    unsigned char n = numfuncs;
    numfuncs = 0;
    while (n > 0) {
      funcs[n-1]();    // Might call atexit.
      n--;
    }
  }
  _Exit(status);
}

