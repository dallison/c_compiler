//
//  perror.c
//  c_compiler
//
//  Created by David Allison on 12/22/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdio.h>
#include <errno.h>

void perror(const char* s) {
  const char* e = strerror(errno);
  if (s != NULL && *s != '\0') {
    fprintf(stderr, "%s: %s\n", s, e);
  } else {
    printf("%s\n", e);
  }
}
