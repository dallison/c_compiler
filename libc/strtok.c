//
//  strtok.c
//  c_compiler
//
//  Created by David Allison on 6/20/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include <string.h>
#include <stdbool.h>


char* strtok_r(char* restrict string, const char* restrict delimiters,
               char** restrict save) {
  if (string == NULL) {
    string = *save;
  }
  if (string == NULL) {
    return NULL;
  }

  string += strspn(string, delimiters);
  if (*string == '\0') {
    *save = string;
    return NULL;
  }

  char* token = string;
  string += strcspn(string, delimiters);
  if (*string != '\0') {
    *string++ = '\0';
  }
  *save = string;
  return token;
}

char* strtok(char* restrict string, const char* restrict delimiters) {
  static char* saved;
  return strtok_r(string, delimiters, &saved);
}
