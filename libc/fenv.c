#include <fenv.h>

const fenv_t __davecc_fenv_default = {0, FE_TONEAREST};

typedef struct {
  int __round;
} __davecc_femode_t;

const __davecc_femode_t __davecc_femode_default = {FE_TONEAREST};

#if defined(__6502__) || defined(__W65C02__) || defined(__p_code__) || \
    defined(__wasm32__)
static fenv_t __davecc_current_fenv = {0, FE_TONEAREST};
#else
static __thread fenv_t __davecc_current_fenv = {0, FE_TONEAREST};
#endif

static int IsValidRound(int round) {
  return round == FE_TONEAREST || round == FE_DOWNWARD ||
         round == FE_UPWARD || round == FE_TOWARDZERO;
}

int feclearexcept(int excepts) {
  __davecc_current_fenv.__exceptions &= ~(fexcept_t)(excepts & FE_ALL_EXCEPT);
  return 0;
}

int fegetexceptflag(fexcept_t* flagp, int excepts) {
  if (flagp == 0) return 1;
  *flagp = __davecc_current_fenv.__exceptions &
           (fexcept_t)(excepts & FE_ALL_EXCEPT);
  return 0;
}

int feraiseexcept(int excepts) {
  __davecc_current_fenv.__exceptions |=
      (fexcept_t)(excepts & FE_ALL_EXCEPT);
  return 0;
}

int fesetexceptflag(const fexcept_t* flagp, int excepts) {
  if (flagp == 0) return 1;
  fexcept_t mask = (fexcept_t)(excepts & FE_ALL_EXCEPT);
  __davecc_current_fenv.__exceptions =
      (__davecc_current_fenv.__exceptions & ~mask) | (*flagp & mask);
  return 0;
}

int fetestexcept(int excepts) {
  return (int)(__davecc_current_fenv.__exceptions &
               (fexcept_t)(excepts & FE_ALL_EXCEPT));
}

int fegetround(void) {
  return __davecc_current_fenv.__round;
}

int fesetround(int round) {
  if (!IsValidRound(round)) return 1;
  __davecc_current_fenv.__round = round;
  return 0;
}

int fegetenv(fenv_t* envp) {
  if (envp == 0) return 1;
  *envp = __davecc_current_fenv;
  return 0;
}

int feholdexcept(fenv_t* envp) {
  if (fegetenv(envp) != 0) return 1;
  __davecc_current_fenv.__exceptions = 0;
  return 0;
}

int fesetenv(const fenv_t* envp) {
  if (envp == 0 || !IsValidRound(envp->__round)) return 1;
  __davecc_current_fenv = *envp;
  __davecc_current_fenv.__exceptions &= FE_ALL_EXCEPT;
  return 0;
}

int feupdateenv(const fenv_t* envp) {
  fexcept_t raised = __davecc_current_fenv.__exceptions;
  if (fesetenv(envp) != 0) return 1;
  __davecc_current_fenv.__exceptions |= raised;
  return 0;
}

int fegetmode(__davecc_femode_t* modep) {
  if (modep == 0) return 1;
  modep->__round = __davecc_current_fenv.__round;
  return 0;
}

int fesetmode(const __davecc_femode_t* modep) {
  if (modep == 0 || !IsValidRound(modep->__round)) return 1;
  __davecc_current_fenv.__round = modep->__round;
  return 0;
}

int fesetexcept(int excepts) {
  __davecc_current_fenv.__exceptions |=
      (fexcept_t)(excepts & FE_ALL_EXCEPT);
  return 0;
}

int fetestexceptflag(const fexcept_t* flagp, int excepts) {
  if (flagp == 0) return 0;
  return (int)(*flagp & (fexcept_t)(excepts & FE_ALL_EXCEPT));
}
