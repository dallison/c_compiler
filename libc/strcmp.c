//
//  strcmp.c
//  c_compiler
//
//  Created by David Allison on 4/30/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

int strcmp(const char* a, const char* b) {
  while (*a != '\0' && *b != '\0') {
    if (*a != *b) {
      break;
    }
  }
  return *a - *b;
}
