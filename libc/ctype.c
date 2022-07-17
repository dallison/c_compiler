//
//  ctype.c
//  c_compiler
//
//  Created by David Allison on 2/8/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include <ctype.h>

#define A (1 << 0)    // alpha
#define N (1 << 1)    // number
#define S (1 << 2)    // space
#define P (1 << 3)    // punctuation
#define C (1 << 4)    // control
#define L (1 << 5)    // lower
#define U (1 << 6)    // upper
#define X (1 << 7)    // xdigit


static char char_traits[256] = {
  //       00      01      02      03      04      05      06      07
  //       08      09      0a      0b      0c      0d      0e      0f
  /* 00 */ C,      C,      C,      C,      C,      C,      C,      C,
  /* 08 */ C,      C|S,    C|S,    C|S,    C|S,    C|S,    C,      C,
  /* 10 */ C,      C,      C,      C,      C,      C,      C,      C,
  /* 18 */ C,      C,      C,      C,      C,      C,      C,      C,
  /* 20 */ S,      P,      P,      P,      P,      P,      P,      P,
  /* 28 */ P,      P,      P,      P,      P,      P,      P,      P,
  /* 30 */ N|X,    N|X,    N|X,    N|X,    N|X,    N|X,    N|X,    N|X,
  /* 38 */ N|X,    N|X,    P,      P,      P,      P,      P,      P,
  /* 40 */ P,      A|X|U,  A|X|U,  A|X|U,  A|X|U,  A|X|U,  A|X|U,  A|U,
  /* 48 */ A|U,    A|U,    A|U,    A|U,    A|U,    A|U,    A|U,    A|U,
  /* 50 */ A|U,    A|U,    A|U,    A|U,    A|U,    A|U,    A|U,    A|U,
  /* 58 */ A|U,    A|U,    A|U,    P,      P,      P,      P,      P,
  /* 60 */ P,      A|L|X,  A|L|X,  A|L|X,  A|L|X,  A|L|X,  A|L|X,  A|L,
  /* 60 */ A|L,    A|L,    A|L,    A|L,    A|L,    A|L,    A|L,    A|L,
  /* 70 */ A|L,    A|L,    A|L,    A|L,    A|L,    A|L,    A|L,    A|L,
  /* 78 */ A|L,    A|L,    A|L,    P,      P,      P,      P,      C,
  /* 80 */ 0,      0,      0,      0,      0,      0,      0,      0,
  /* 88 */ 0,      0,      0,      0,      0,      0,      0,      0,
  /* 90 */ 0,      0,      0,      0,      0,      0,      0,      0,
  /* 98 */ 0,      0,      0,      0,      0,      0,      0,      0,
  /* a0 */ 0,      0,      0,      0,      0,      0,      0,      0,
  /* a8 */ 0,      0,      0,      0,      0,      0,      0,      0,
  /* b0 */ 0,      0,      0,      0,      0,      0,      0,      0,
  /* b8 */ 0,      0,      0,      0,      0,      0,      0,      0,
  /* c0 */ 0,      0,      0,      0,      0,      0,      0,      0,
  /* c8 */ 0,      0,      0,      0,      0,      0,      0,      0,
  /* d0 */ 0,      0,      0,      0,      0,      0,      0,      0,
  /* d8 */ 0,      0,      0,      0,      0,      0,      0,      0,
  /* e0 */ 0,      0,      0,      0,      0,      0,      0,      0,
  /* e8 */ 0,      0,      0,      0,      0,      0,      0,      0,
  /* f0 */ 0,      0,      0,      0,      0,      0,      0,      0,
  /* f8 */ 0,      0,      0,      0,      0,      0,      0,      0,
};

int isalnum(int c) {
  int ch = c & 0xff;
  return (char_traits[ch] & (A|N)) != 0;
}

int isalpha(int c) {
  int ch = c & 0xff;
  return (char_traits[ch] & A) != 0;
}

int isblank(int c) {
  return c == 0x09 || c == ' ';
}

int iscntrl(int c) {
  int ch = c & 0xff;
  return (char_traits[ch] & C) != 0;
}

int isdigit(int c) {
  int ch = c & 0xff;
  return (char_traits[ch] & N) != 0;
}

int isgraph(int c) {
  return c != ' ';
}

int islower(int c) {
  int ch = c & 0xff;
  return (char_traits[ch] & L) != 0;
}

int isprint(int c) {
  int ch = c & 0xff;
  return (char_traits[ch] & (A|N|S|P)) != 0;
}

int ispunct(int c) {
  int ch = c & 0xff;
  return (char_traits[ch] & P) != 0;
}

int isspace(int c) {
  int ch = c & 0xff;
  return (char_traits[ch] & S) != 0;
}

int isupper(int c) {
  int ch = c & 0xff;
  return (char_traits[ch] & U) != 0;
}

int isxdigit(int c) {
  int ch = c & 0xff;
  return (char_traits[ch] & X) != 0;
}

int tolower(int c) {
  if (isupper(c)) {
    return c - 'A' + 'a';
  }
  return c;
}

int toupper(int c) {
  if (islower(c)) {
    return c - 'a' + 'A';
  }
  return c;
}
