#ifndef davecc_dirent_h
#define davecc_dirent_h

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

#endif
