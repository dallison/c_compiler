#ifndef _SYS_UCONTEXT_H
#define _SYS_UCONTEXT_H

#if defined(__DAVECC_NATIVE_DARWIN__)
// The SDK's signal headers already define mcontext_t as a pointer to its
// machine context.
#include_next <sys/ucontext.h>
#else
#include <ucontext.h>
#endif  // __DAVECC_NATIVE_DARWIN__

#endif
