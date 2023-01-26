//
//  main.c
//  stdio_test
//
//  Created by David Allison on 12/13/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include "daveccdefs.h"

int main(int argc, const char * argv[]) {
  fwrite("foo", 1, 3, stdout);
  fflush(stdout);
  
  FILE* fp = fopen("/tmp/foo", "w");
  fwrite("foo", 1, 3, fp);
  fclose(fp);
  
  fp = fopen("/tmp/foo", "w");
  fputc('x', fp);
  fclose(fp);
  
  fputs("foobar\n", stderr);
  
  char ch = getchar();
  
  fp = fopen("/tmp/foo", "w");
  fseek(fp, 1, SEEK_SET);
  fputc(ch, fp);
  fclose(fp);
  
  fp = fopen("/tmp/foo", "r");
  int x = fgetc(fp);
  if (x == 'x') {
    
  }
  int y = fgetc(fp);
  if (y == 'x') {
    
  }
  ungetc('a', fp);
  y = fgetc(fp);
  
  char buf[10];
  fseek(fp, 0, SEEK_SET);
  fread(buf, 1, 10, fp);
  
  fclose(fp);
}
