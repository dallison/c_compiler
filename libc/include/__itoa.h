// Compact integer-to-ASCII conversion helpers.
//
// These produce the same digit strings as snprintf's %d/%u/%x/%X/%o family
// (including the showbase prefixes) without pulling the full printf
// formatting machinery into the link.  iostream and std::to_string use them
// so that programs which never call printf don't pay for it.
//
// Each width lives in its own translation unit so the static linker only
// extracts the conversions a program actually uses.

#ifndef __davecc_itoa_h
#define __davecc_itoa_h

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Flag bits accepted by the conversion functions.
#define __DAVECC_ITOA_UPPER 1u     // Use ABCDEF and 0X for hex.
#define __DAVECC_ITOA_SHOWBASE 2u  // Prefix 0x/0X (hex) or 0 (octal) if != 0.
#define __DAVECC_ITOA_SHOWPOS 4u   // Prefix '+' (callers pass for signed dec).
#define __DAVECC_ITOA_NEGATIVE 8u  // Internal: prefix '-'.

// Worst case output: octal digits + base prefix + sign.  No NUL is written;
// all functions return the number of characters produced.
#define __DAVECC_ITOA_CAPACITY(type) ((sizeof(type) * 8 + 2) / 3 + 3)

size_t __utoa_uint(char* buffer, unsigned int value, unsigned char base,
                   unsigned char flags);
size_t __itoa_int(char* buffer, int value, unsigned char base,
                  unsigned char flags);

size_t __utoa_ulong(char* buffer, unsigned long value, unsigned char base,
                    unsigned char flags);
size_t __itoa_long(char* buffer, long value, unsigned char base,
                   unsigned char flags);

size_t __utoa_ulonglong(char* buffer, unsigned long long value,
                        unsigned char base, unsigned char flags);
size_t __itoa_longlong(char* buffer, long long value, unsigned char base,
                       unsigned char flags);

// Formats like %p: "0x" followed by lowercase hex digits.
size_t __ptrtoa(char* buffer, const void* value);

#ifdef __cplusplus
}
#endif

#endif  // __davecc_itoa_h
