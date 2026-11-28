#ifndef davecc_shadow_h
#define davecc_shadow_h

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct spwd {
  char* sp_namp;             /* login name */
  char* sp_pwdp;             /* encrypted password */
  long sp_lstchg;            /* date of last change (days since epoch) */
  long sp_min;               /* min days between changes */
  long sp_max;               /* max days between changes */
  long sp_warn;              /* days before password expires to warn */
  long sp_inact;             /* days after expire until account disabled */
  long sp_expire;            /* account expiration day */
  unsigned long sp_flag;     /* reserved */
};

struct spwd* getspnam(const char* name);
struct spwd* getspent(void);
void setspent(void);
void endspent(void);

#ifdef __cplusplus
}
#endif

#endif
