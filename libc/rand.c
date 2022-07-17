//
//  rand.c
//  c_compiler
//
//  Created by David Allison on 6/29/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include <stdlib.h>

// This is the portable rand/srand from the C99 spec.
static unsigned long int next = 1;

int rand(void) { // RAND_MAX assumed to be 32767
    next = next * 1103515245 + 12345;
    return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
    next = seed;
}
