// Build the standard printf engine without floating-point conversions.  This
// keeps long and long-long formatting in a separate archive member, so using
// one does not pull ftoa and its fixed-point support into the program.
#define PRINTF_DISABLE_FLOAT
#define PRINTF_SPECIALIZED_LONG
#include "printf.c"
