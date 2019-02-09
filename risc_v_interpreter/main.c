//
//  main.c
//  risc_v_interpreter
//
//  Created by David Allison on 4/23/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include <stdio.h>
#include "loader.h"
#include "risc_v_interpreter.h"
#include <stdlib.h>

int main(int argc, char * argv[]) {
  String filename;
  StringInit(&filename, argv[1]);
  
  Loader loader;
  bool ok = LoaderInitFromFile(&loader, &filename);
  if (!ok) {
    printf("Error Loading %s\n", filename.value);
    exit(1);
  }
  
  Interpreter interpreter;
  InterpreterInit(&interpreter, &loader, loader.main_address, argc, argv);
  InterpreterRun(&interpreter);
  
  InterpreterDestruct(&interpreter);
  LoaderDestruct(&loader);
  
}
