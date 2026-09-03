#ifndef __davecc_fenv_h
#define __davecc_fenv_h

#if !defined(__cplusplus) && \
    (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L)
#error "<fenv.h> requires C99 or later"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int fexcept_t;

typedef struct {
  fexcept_t __exceptions;
  int __round;
} fenv_t;

#define FE_INVALID 0x01
#define FE_DIVBYZERO 0x02
#define FE_OVERFLOW 0x04
#define FE_UNDERFLOW 0x08
#define FE_INEXACT 0x10
#define FE_ALL_EXCEPT \
  (FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW | FE_UNDERFLOW | FE_INEXACT)

#define FE_TONEAREST 0
#define FE_DOWNWARD 1
#define FE_UPWARD 2
#define FE_TOWARDZERO 3

extern const fenv_t __davecc_fenv_default;
#define FE_DFL_ENV (&__davecc_fenv_default)

int feclearexcept(int excepts);
int fegetexceptflag(fexcept_t* flagp, int excepts);
int feraiseexcept(int excepts);
int fesetexceptflag(const fexcept_t* flagp, int excepts);
int fetestexcept(int excepts);
int fegetround(void);
int fesetround(int round);
int fegetenv(fenv_t* envp);
int feholdexcept(fenv_t* envp);
int fesetenv(const fenv_t* envp);
int feupdateenv(const fenv_t* envp);

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202000L
typedef struct {
  int __round;
} femode_t;

extern const femode_t __davecc_femode_default;
#define FE_DFL_MODE (&__davecc_femode_default)

int fegetmode(femode_t* modep);
int fesetmode(const femode_t* modep);
int fesetexcept(int excepts);
int fetestexceptflag(const fexcept_t* flagp, int excepts);
#endif

#ifdef __cplusplus
}
#endif

#endif
