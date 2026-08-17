#ifndef _STD_NORETURN_H
#define _STD_NORETURN_H

#ifndef __cplusplus
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L
#error "<stdnoreturn.h> requires C11 or later"
#endif
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define noreturn [[__noreturn__]]
#else
#define noreturn _Noreturn
#endif
#endif

#endif
