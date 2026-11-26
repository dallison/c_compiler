/*
 * POSIX spawn(3) — attribute / file-action API and posix_spawn{,p}.
 */
#ifndef davecc_spawn_h
#define davecc_spawn_h

#if defined(__DAVECC_NATIVE_DARWIN__)
#include_next <spawn.h>
#else

#include <signal.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define POSIX_SPAWN_RESETIDS 0x01
#define POSIX_SPAWN_SETPGROUP 0x02
#define POSIX_SPAWN_SETSIGDEF 0x04
#define POSIX_SPAWN_SETSIGMASK 0x08
#define POSIX_SPAWN_SETSCHEDPARAM 0x10
#define POSIX_SPAWN_SETSCHEDULER 0x20
#define POSIX_SPAWN_USEVFORK 0x40
#define POSIX_SPAWN_SETSID 0x80

typedef struct {
  short __flags;
  pid_t __pgroup;
  sigset_t __sd;
  sigset_t __ss;
  int __policy;
  int __prio;
} posix_spawnattr_t;

struct __posix_spawn_file_action {
  int __type;
  int __fildes;
  int __arg;
  mode_t __mode;
  char* __path;
};

typedef struct {
  struct __posix_spawn_file_action* __acts;
  unsigned __n;
  unsigned __cap;
} posix_spawn_file_actions_t;

int posix_spawn(pid_t* pid, const char* path,
                const posix_spawn_file_actions_t* file_actions,
                const posix_spawnattr_t* attrp, char* const argv[],
                char* const envp[]);
int posix_spawnp(pid_t* pid, const char* file,
                 const posix_spawn_file_actions_t* file_actions,
                 const posix_spawnattr_t* attrp, char* const argv[],
                 char* const envp[]);

int posix_spawnattr_init(posix_spawnattr_t* attr);
int posix_spawnattr_destroy(posix_spawnattr_t* attr);
int posix_spawnattr_getflags(const posix_spawnattr_t* attr, short* flags);
int posix_spawnattr_setflags(posix_spawnattr_t* attr, short flags);
int posix_spawnattr_getpgroup(const posix_spawnattr_t* attr, pid_t* pgroup);
int posix_spawnattr_setpgroup(posix_spawnattr_t* attr, pid_t pgroup);
int posix_spawnattr_getsigdefault(const posix_spawnattr_t* attr,
                                  sigset_t* sigdefault);
int posix_spawnattr_setsigdefault(posix_spawnattr_t* attr,
                                  const sigset_t* sigdefault);
int posix_spawnattr_getsigmask(const posix_spawnattr_t* attr,
                               sigset_t* sigmask);
int posix_spawnattr_setsigmask(posix_spawnattr_t* attr,
                               const sigset_t* sigmask);

int posix_spawn_file_actions_init(posix_spawn_file_actions_t* actions);
int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t* actions);
int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t* actions,
                                      int fildes);
int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t* actions,
                                     int fildes, int newfildes);
int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t* actions,
                                     int fildes, const char* path, int oflag,
                                     mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* !__DAVECC_NATIVE_DARWIN__ */
#endif /* davecc_spawn_h */
