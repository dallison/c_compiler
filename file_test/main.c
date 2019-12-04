//
//  main.c
//  file_test
//
//  Created by David Allison on 11/15/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include <stdio.h>
#include "compiler.h"

int main(int argc, char * argv[]) {
  Vector options;
  ParseOptions(argc, argv, &options);
  for (size_t i = 0; i < options.length; i++) {
    CompilerOptionValue* opt = options.value.p[i];
    if (opt->opt == kOptionInputFile) {
      CompileTranslationUnit(opt->value.svalue.value, &options);
    }
  }
}
