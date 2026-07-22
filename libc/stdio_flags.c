// feof/ferror/clearerr in their own translation unit so they only link when
// used.

#include <stdio.h>

int feof(FILE* stream) { return stream->eof_flag; }

int ferror(FILE* stream) { return stream->error_flag; }

void clearerr(FILE* stream) { stream->error_flag = 0; }
