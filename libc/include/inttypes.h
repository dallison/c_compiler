//
//  inttypes.h
//  c_compiler
//
//  Created by David Allison on 5/9/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef inttypes_h
#define inttypes_h

#include <stdint.h>

# if __WORDSIZE == 64
#  define __PRI64_PREFIX  "l"
#  define __PRIPTR_PREFIX  "l"
# else
#  define __PRI64_PREFIX  "ll"
#  define __PRIPTR_PREFIX
# endif

# if defined(__6502__)
# define __PRI_INT_PREFIX "l"
# define __SCN_INT_PREFIX "l"
# define __SCN_SHORT_PREFIX
# else
# define __PRI_INT_PREFIX
# define __SCN_INT_PREFIX
# define __SCN_SHORT_PREFIX "h"
# endif

/* Macros for printing format specifiers.  */

/* Decimal notation.  */
# define PRId8    "d"
# define PRId16    "d"
# define PRId32    __PRI_INT_PREFIX "d"
# define PRId64    __PRI64_PREFIX "d"

# define PRIdLEAST8  "d"
# define PRIdLEAST16  "d"
# define PRIdLEAST32  __PRI_INT_PREFIX "d"
# define PRIdLEAST64  __PRI64_PREFIX "d"

# define PRIdFAST8  "d"
# define PRIdFAST16  __PRIPTR_PREFIX "d"
# define PRIdFAST32  __PRIPTR_PREFIX "d"
# define PRIdFAST64  __PRI64_PREFIX "d"


# define PRIi8    "i"
# define PRIi16    "i"
# define PRIi32    __PRI_INT_PREFIX "i"
# define PRIi64    __PRI64_PREFIX "i"

# define PRIiLEAST8  "i"
# define PRIiLEAST16  "i"
# define PRIiLEAST32  __PRI_INT_PREFIX "i"
# define PRIiLEAST64  __PRI64_PREFIX "i"

# define PRIiFAST8  "i"
# define PRIiFAST16  __PRIPTR_PREFIX "i"
# define PRIiFAST32  __PRIPTR_PREFIX "i"
# define PRIiFAST64  __PRI64_PREFIX "i"

/* Octal notation.  */
# define PRIo8    "o"
# define PRIo16    "o"
# define PRIo32    __PRI_INT_PREFIX "o"
# define PRIo64    __PRI64_PREFIX "o"

# define PRIoLEAST8  "o"
# define PRIoLEAST16  "o"
# define PRIoLEAST32  __PRI_INT_PREFIX "o"
# define PRIoLEAST64  __PRI64_PREFIX "o"

# define PRIoFAST8  "o"
# define PRIoFAST16  __PRIPTR_PREFIX "o"
# define PRIoFAST32  __PRIPTR_PREFIX "o"
# define PRIoFAST64  __PRI64_PREFIX "o"

/* Unsigned integers.  */
# define PRIu8    "u"
# define PRIu16    "u"
# define PRIu32    __PRI_INT_PREFIX "u"
# define PRIu64    __PRI64_PREFIX "u"

# define PRIuLEAST8  "u"
# define PRIuLEAST16  "u"
# define PRIuLEAST32  __PRI_INT_PREFIX "u"
# define PRIuLEAST64  __PRI64_PREFIX "u"

# define PRIuFAST8  "u"
# define PRIuFAST16  __PRIPTR_PREFIX "u"
# define PRIuFAST32  __PRIPTR_PREFIX "u"
# define PRIuFAST64  __PRI64_PREFIX "u"

/* lowercase hexadecimal notation.  */
# define PRIx8    "x"
# define PRIx16    "x"
# define PRIx32    __PRI_INT_PREFIX "x"
# define PRIx64    __PRI64_PREFIX "x"

# define PRIxLEAST8  "x"
# define PRIxLEAST16  "x"
# define PRIxLEAST32  __PRI_INT_PREFIX "x"
# define PRIxLEAST64  __PRI64_PREFIX "x"

# define PRIxFAST8  "x"
# define PRIxFAST16  __PRIPTR_PREFIX "x"
# define PRIxFAST32  __PRIPTR_PREFIX "x"
# define PRIxFAST64  __PRI64_PREFIX "x"

/* UPPERCASE hexadecimal notation.  */
# define PRIX8    "X"
# define PRIX16    "X"
# define PRIX32    __PRI_INT_PREFIX "X"
# define PRIX64    __PRI64_PREFIX "X"

# define PRIXLEAST8  "X"
# define PRIXLEAST16  "X"
# define PRIXLEAST32  __PRI_INT_PREFIX "X"
# define PRIXLEAST64  __PRI64_PREFIX "X"

# define PRIXFAST8  "X"
# define PRIXFAST16  __PRIPTR_PREFIX "X"
# define PRIXFAST32  __PRIPTR_PREFIX "X"
# define PRIXFAST64  __PRI64_PREFIX "X"


/* Macros for printing `intmax_t' and `uintmax_t'.  */
# define PRIdMAX  __PRI64_PREFIX "d"
# define PRIiMAX  __PRI64_PREFIX "i"
# define PRIoMAX  __PRI64_PREFIX "o"
# define PRIuMAX  __PRI64_PREFIX "u"
# define PRIxMAX  __PRI64_PREFIX "x"
# define PRIXMAX  __PRI64_PREFIX "X"


/* Macros for printing `intptr_t' and `uintptr_t'.  */
# define PRIdPTR  __PRIPTR_PREFIX "d"
# define PRIiPTR  __PRIPTR_PREFIX "i"
# define PRIoPTR  __PRIPTR_PREFIX "o"
# define PRIuPTR  __PRIPTR_PREFIX "u"
# define PRIxPTR  __PRIPTR_PREFIX "x"
# define PRIXPTR  __PRIPTR_PREFIX "X"


/* Macros for scanning format specifiers.  */

/* Signed decimal notation.  */
# define SCNd8    "hhd"
# define SCNd16    __SCN_SHORT_PREFIX "d"
# define SCNd32    __SCN_INT_PREFIX "d"
# define SCNd64    __PRI64_PREFIX "d"

# define SCNdLEAST8  "hhd"
# define SCNdLEAST16  __SCN_SHORT_PREFIX "d"
# define SCNdLEAST32  __SCN_INT_PREFIX "d"
# define SCNdLEAST64  __PRI64_PREFIX "d"

# define SCNdFAST8  "hhd"
# define SCNdFAST16  __PRIPTR_PREFIX "d"
# define SCNdFAST32  __PRIPTR_PREFIX "d"
# define SCNdFAST64  __PRI64_PREFIX "d"

/* Signed decimal notation.  */
# define SCNi8    "hhi"
# define SCNi16    __SCN_SHORT_PREFIX "i"
# define SCNi32    __SCN_INT_PREFIX "i"
# define SCNi64    __PRI64_PREFIX "i"

# define SCNiLEAST8  "hhi"
# define SCNiLEAST16  __SCN_SHORT_PREFIX "i"
# define SCNiLEAST32  __SCN_INT_PREFIX "i"
# define SCNiLEAST64  __PRI64_PREFIX "i"

# define SCNiFAST8  "hhi"
# define SCNiFAST16  __PRIPTR_PREFIX "i"
# define SCNiFAST32  __PRIPTR_PREFIX "i"
# define SCNiFAST64  __PRI64_PREFIX "i"

/* Unsigned decimal notation.  */
# define SCNu8    "hhu"
# define SCNu16    __SCN_SHORT_PREFIX "u"
# define SCNu32    __SCN_INT_PREFIX "u"
# define SCNu64    __PRI64_PREFIX "u"

# define SCNuLEAST8  "hhu"
# define SCNuLEAST16  __SCN_SHORT_PREFIX "u"
# define SCNuLEAST32  __SCN_INT_PREFIX "u"
# define SCNuLEAST64  __PRI64_PREFIX "u"

# define SCNuFAST8  "hhu"
# define SCNuFAST16  __PRIPTR_PREFIX "u"
# define SCNuFAST32  __PRIPTR_PREFIX "u"
# define SCNuFAST64  __PRI64_PREFIX "u"

/* Octal notation.  */
# define SCNo8    "hho"
# define SCNo16    __SCN_SHORT_PREFIX "o"
# define SCNo32    __SCN_INT_PREFIX "o"
# define SCNo64    __PRI64_PREFIX "o"

# define SCNoLEAST8  "hho"
# define SCNoLEAST16  __SCN_SHORT_PREFIX "o"
# define SCNoLEAST32  __SCN_INT_PREFIX "o"
# define SCNoLEAST64  __PRI64_PREFIX "o"

# define SCNoFAST8  "hho"
# define SCNoFAST16  __PRIPTR_PREFIX "o"
# define SCNoFAST32  __PRIPTR_PREFIX "o"
# define SCNoFAST64  __PRI64_PREFIX "o"

/* Hexadecimal notation.  */
# define SCNx8    "hhx"
# define SCNx16    __SCN_SHORT_PREFIX "x"
# define SCNx32    __SCN_INT_PREFIX "x"
# define SCNx64    __PRI64_PREFIX "x"

# define SCNxLEAST8  "hhx"
# define SCNxLEAST16  __SCN_SHORT_PREFIX "h"
# define SCNxLEAST32  __SCN_INT_PREFIX "x"
# define SCNxLEAST64  __PRI64_PREFIX "x"

# define SCNxFAST8  "hhx"
# define SCNxFAST16  __PRIPTR_PREFIX "x"
# define SCNxFAST32  __PRIPTR_PREFIX "x"
# define SCNxFAST64  __PRI64_PREFIX "x"


/* Macros for scanning `intmax_t' and `uintmax_t'.  */
# define SCNdMAX  __PRI64_PREFIX "d"
# define SCNiMAX  __PRI64_PREFIX "i"
# define SCNoMAX  __PRI64_PREFIX "o"
# define SCNuMAX  __PRI64_PREFIX "u"
# define SCNxMAX  __PRI64_PREFIX "x"

/* Macros for scaning `intptr_t' and `uintptr_t'.  */
# define SCNdPTR  __PRIPTR_PREFIX "d"
# define SCNiPTR  __PRIPTR_PREFIX "i"
# define SCNoPTR  __PRIPTR_PREFIX "o"
# define SCNuPTR  __PRIPTR_PREFIX "u"
# define SCNxPTR  __PRIPTR_PREFIX "x"

#endif /* inttypes_h */
