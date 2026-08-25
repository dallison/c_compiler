// RUN: -std=c++11
// GNU/Clang interoperability: type-limit macros and __typeof.

int ok_int[__INT_MAX__ >= 32767 ? 1 : -1];
int ok_long[__LONG_MAX__ >= __INT_MAX__ ? 1 : -1];
int ok_ll[__LONG_LONG_MAX__ >= __LONG_MAX__ ? 1 : -1];
int ok_char_bit[__CHAR_BIT__ == 8 ? 1 : -1];
int ok_sizeof_int[__SIZEOF_INT__ >= 2 ? 1 : -1];
int ok_endian[__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ ? 1 : -1];

enum E { v0, v1 = __INT_MAX__ };

typedef __typeof(+v0) Promoted;
typedef __typeof__(int) SameInt;

int n;
__typeof(n) m = 0;
__const int c = 1;
__inline int f(void) { return __alignof(int) > 0 ? 1 : 0; }

int use(Promoted p, SameInt s) { return (int)p + s + m + c + f(); }
