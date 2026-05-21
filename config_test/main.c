//
//  main.c
//  config_test
//
//  Created by David Allison on 4/2/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include "config.h"

int main(int argc, const char * argv[]) {
  ConfigParser parser;
  ConfigParserInit(&parser, argv[1]);
  if (!ConfigParserParse(&parser)) {
    fprintf(stderr, "Failed to parse\n");
    exit(1);
  }
  
  ConfigParserPrint(&parser, stdout);

}
