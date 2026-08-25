// RUN: -std=c++11
#ifndef __INT_MAX__
#error "__INT_MAX__ must be predefined"
#endif
#ifndef __LONG_MAX__
#error "__LONG_MAX__ must be predefined"
#endif
#ifndef __LONG_LONG_MAX__
#error "__LONG_LONG_MAX__ must be predefined"
#endif
#ifndef __CHAR_BIT__
#error "__CHAR_BIT__ must be predefined"
#endif
#ifndef __SIZEOF_INT__
#error "__SIZEOF_INT__ must be predefined"
#endif
#ifndef __SIZEOF_POINTER__
#error "__SIZEOF_POINTER__ must be predefined"
#endif
#ifndef __SIZE_MAX__
#error "__SIZE_MAX__ must be predefined"
#endif
#ifndef __BYTE_ORDER__
#error "__BYTE_ORDER__ must be predefined"
#endif

#if __CHAR_BIT__ != 8
#error "__CHAR_BIT__ must be 8"
#endif
#if __INT_MAX__ < 32767
#error "__INT_MAX__ is too small"
#endif
#if __LONG_MAX__ < __INT_MAX__
#error "__LONG_MAX__ must be at least __INT_MAX__"
#endif
#if __SIZEOF_INT__ < 2
#error "__SIZEOF_INT__ is too small"
#endif
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "DaveCC targets are little-endian"
#endif
#if __INT32_MAX__ != 2147483647
#error "__INT32_MAX__ must be 2147483647"
#endif
