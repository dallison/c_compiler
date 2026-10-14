#ifndef _ALLOCA_H
#define _ALLOCA_H

#include <stddef.h>

#undef alloca
#define alloca(size) __builtin_alloca(size)

#endif  // _ALLOCA_H
