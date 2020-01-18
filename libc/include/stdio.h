//
//  stdio.h
//  c_compiler
//
//  Created by David Allison on 1/11/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef stdio_h
#define stdio_h

#define EOF (-1)
#define NULL 0

typedef struct {
  int fd;
  char* buf;
  int bufsize;
  int index;
} FILE;

extern FILE* stdout;
extern FILE* stdin;
extern FILE* stderr;

FILE* fopen(const char* filename, const char* mode);
void fclose(FILE* fp);
int fflush(FILE* fp);


#endif /* stdio_h */
