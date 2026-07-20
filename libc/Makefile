default: libc_test

CC = davecc
LIB = libc.so

CFLAGS = -target risc-v -fPIC -Xkeep-asm -Werror -O2
CXX_SRCS = $(wildcard *.cc)
CXX_OBJS = $(patsubst %.cc,cxx_%.o,$(CXX_SRCS))

OBJS = \
  posix.o \
  syscall.o \
  ctype.o \
  fclose.o \
  ffuncs.o \
  fopen.o \
  malloc.o \
  memcmp.o \
  memcpy.o \
  memmove.o \
  memset.o \
  printf.o \
  printf_common.o \
  printf_literal.o \
  printf_long.o \
  strcat.o \
  strchr.o \
  strcmp.o \
  strcpy.o \
  strlen.o \
  strncat.o \
  strncmp.o \
  strncpy.o \
  strrchr.o \
  strstr.o \
  strtol.o \
  strings.o \
  $(CXX_OBJS)

cxx_%.o: %.cc
	$(CC) $(CFLAGS) -std=c++20 -c $< -o $@

$(LIB): $(OBJS)
	$(CC) -o $(LIB) -shared $(OBJS)

libc_test: libc_test.o $(LIB)
	$(CC) -o libc_test libc_test.o -L. -lc -rpath "/Users/dallison/Google Drive/c_compiler/libc"

clean:
	rm -f $(LIB) $(OBJS) libc_test.o

