#ifndef davecc_dirent_h
#define davecc_dirent_h

#if defined(__DAVECC_NATIVE_DARWIN__)
// libSystem's readdir fills the SDK's struct dirent.  The guest layout below
// puts d_name at a different offset, so native code would skip every entry.
#include_next <dirent.h>
#else

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

struct dirent {
  unsigned long d_ino;
  char d_name[NAME_MAX + 1];
};

typedef struct DaveccDir DIR;

#ifdef __cplusplus
extern "C" {
#endif

DIR* opendir(const char* name);
struct dirent* readdir(DIR* directory);
int closedir(DIR* directory);
int dirfd(DIR* directory);

#ifdef __cplusplus
}
#endif

#endif /* !__DAVECC_NATIVE_DARWIN__ */

#endif
