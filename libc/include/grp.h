#ifndef davecc_grp_h
#define davecc_grp_h

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct group {
  char* gr_name;
  char* gr_passwd;
  gid_t gr_gid;
  char** gr_mem;
};

struct group* getgrnam(const char* name);
struct group* getgrgid(gid_t gid);
struct group* getgrent(void);
void setgrent(void);
void endgrent(void);

#ifdef __cplusplus
}
#endif

#endif
