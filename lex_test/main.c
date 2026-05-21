//
//  main.c
//  lex_test
//
//  Created by David Allison on 10/27/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include <stdio.h>

#include "lex.h"

int main(int argc, const char * argv[]) {
  Lex lex;
  LexInit(&lex, "/Users/dallison/GoogleDrive/c_compiler_tests/t1.c");
  
  while (!LexEof(&lex)) {
    LexNextToken(&lex);
    printf("token: %s\n", TokenName(lex.current_token));
  }
  LexDestruct(&lex);
}
