//
//  strcpy.c
//  c_compiler
//
//  Created by David Allison on 4/29/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

char* strcpy(char* dest, const char* src) {
  char* p = dest;
  while (*src != '\0') {
    *p++ = *src++;
  }
  *p = '\0';
  return dest;
}
