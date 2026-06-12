//
//  exit.c
//  c_compiler
//
//  Created by David Allison on 6/17/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#if defined(__x86_64__)
void _Exit(int status);

int atexit(void (*p)(void)) {
  (void)p;
  return 0;
}

void exit(int status) {
  _Exit(status);
}

#else

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
  // Flush all open streams so buffered output reaches the OS before we
  // terminate.  Required for normal program termination per the C standard.
  fflush(NULL);
  _Exit(status);
}

#endif
