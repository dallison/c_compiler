/*
 * Userspace posix_spawn{,p}: fork, apply attrs/file actions, exec.
 * Child reports setup/exec failures over a pipe so the parent can return
 * the errno (POSIX spawn returns an error number, not -1).
 */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#if !defined(__DAVECC_NATIVE_DARWIN__)

enum {
  kActClose = 1,
  kActDup2 = 2,
  kActOpen = 3,
};

#ifndef DAVECC_SPAWN_NSIG
#define DAVECC_SPAWN_NSIG 32
#endif

static int actions_grow(posix_spawn_file_actions_t* fa) {
  unsigned ncap = fa->__cap ? fa->__cap * 2u : 4u;
  struct __posix_spawn_file_action* neu =
      (struct __posix_spawn_file_action*)realloc(
          fa->__acts, ncap * sizeof(*neu));
  if (neu == NULL) {
    return ENOMEM;
  }
  fa->__acts = neu;
  fa->__cap = ncap;
  return 0;
}

static int actions_push(posix_spawn_file_actions_t* fa,
                        const struct __posix_spawn_file_action* act) {
  int err;
  if (fa == NULL) {
    return EINVAL;
  }
  if (fa->__n >= fa->__cap) {
    err = actions_grow(fa);
    if (err != 0) {
      return err;
    }
  }
  fa->__acts[fa->__n++] = *act;
  return 0;
}

int posix_spawnattr_init(posix_spawnattr_t* attr) {
  if (attr == NULL) {
    return EINVAL;
  }
  memset(attr, 0, sizeof(*attr));
  return 0;
}

int posix_spawnattr_destroy(posix_spawnattr_t* attr) {
  if (attr == NULL) {
    return EINVAL;
  }
  memset(attr, 0, sizeof(*attr));
  return 0;
}

int posix_spawnattr_getflags(const posix_spawnattr_t* attr, short* flags) {
  if (attr == NULL || flags == NULL) {
    return EINVAL;
  }
  *flags = attr->__flags;
  return 0;
}

int posix_spawnattr_setflags(posix_spawnattr_t* attr, short flags) {
  short known = (short)(POSIX_SPAWN_RESETIDS | POSIX_SPAWN_SETPGROUP |
                        POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK |
                        POSIX_SPAWN_SETSCHEDPARAM | POSIX_SPAWN_SETSCHEDULER |
                        POSIX_SPAWN_USEVFORK | POSIX_SPAWN_SETSID);
  if (attr == NULL || (flags & ~known) != 0) {
    return EINVAL;
  }
  attr->__flags = flags;
  return 0;
}

int posix_spawnattr_getpgroup(const posix_spawnattr_t* attr, pid_t* pgroup) {
  if (attr == NULL || pgroup == NULL) {
    return EINVAL;
  }
  *pgroup = attr->__pgroup;
  return 0;
}

int posix_spawnattr_setpgroup(posix_spawnattr_t* attr, pid_t pgroup) {
  if (attr == NULL) {
    return EINVAL;
  }
  attr->__pgroup = pgroup;
  return 0;
}

int posix_spawnattr_getsigdefault(const posix_spawnattr_t* attr,
                                  sigset_t* sigdefault) {
  if (attr == NULL || sigdefault == NULL) {
    return EINVAL;
  }
  *sigdefault = attr->__sd;
  return 0;
}

int posix_spawnattr_setsigdefault(posix_spawnattr_t* attr,
                                  const sigset_t* sigdefault) {
  if (attr == NULL || sigdefault == NULL) {
    return EINVAL;
  }
  attr->__sd = *sigdefault;
  return 0;
}

int posix_spawnattr_getsigmask(const posix_spawnattr_t* attr,
                               sigset_t* sigmask) {
  if (attr == NULL || sigmask == NULL) {
    return EINVAL;
  }
  *sigmask = attr->__ss;
  return 0;
}

int posix_spawnattr_setsigmask(posix_spawnattr_t* attr,
                               const sigset_t* sigmask) {
  if (attr == NULL || sigmask == NULL) {
    return EINVAL;
  }
  attr->__ss = *sigmask;
  return 0;
}

int posix_spawn_file_actions_init(posix_spawn_file_actions_t* actions) {
  if (actions == NULL) {
    return EINVAL;
  }
  actions->__acts = NULL;
  actions->__n = 0;
  actions->__cap = 0;
  return 0;
}

int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t* actions) {
  unsigned i;
  if (actions == NULL) {
    return EINVAL;
  }
  for (i = 0; i < actions->__n; i++) {
    free(actions->__acts[i].__path);
  }
  free(actions->__acts);
  actions->__acts = NULL;
  actions->__n = 0;
  actions->__cap = 0;
  return 0;
}

int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t* actions,
                                      int fildes) {
  struct __posix_spawn_file_action act;
  if (fildes < 0) {
    return EBADF;
  }
  memset(&act, 0, sizeof(act));
  act.__type = kActClose;
  act.__fildes = fildes;
  return actions_push(actions, &act);
}

int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t* actions,
                                     int fildes, int newfildes) {
  struct __posix_spawn_file_action act;
  if (fildes < 0 || newfildes < 0) {
    return EBADF;
  }
  memset(&act, 0, sizeof(act));
  act.__type = kActDup2;
  act.__fildes = fildes;
  act.__arg = newfildes;
  return actions_push(actions, &act);
}

int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t* actions,
                                     int fildes, const char* path, int oflag,
                                     mode_t mode) {
  struct __posix_spawn_file_action act;
  char* copy;
  if (actions == NULL || path == NULL || fildes < 0) {
    return EINVAL;
  }
  copy = strdup(path);
  if (copy == NULL) {
    return ENOMEM;
  }
  memset(&act, 0, sizeof(act));
  act.__type = kActOpen;
  act.__fildes = fildes;
  act.__arg = oflag;
  act.__mode = mode;
  act.__path = copy;
  {
    int err = actions_push(actions, &act);
    if (err != 0) {
      free(copy);
    }
    return err;
  }
}

static void child_fail(int errfd, int err) {
  if (errfd >= 0) {
    (void)write(errfd, &err, sizeof(err));
  }
  _exit(127);
}

static void apply_attrs(const posix_spawnattr_t* attr, int errfd) {
  short flags;
  if (attr == NULL) {
    return;
  }
  flags = attr->__flags;
  if ((flags & (POSIX_SPAWN_SETSCHEDPARAM | POSIX_SPAWN_SETSCHEDULER |
                POSIX_SPAWN_SETPGROUP)) != 0) {
    child_fail(errfd, ENOTSUP);
  }
  if ((flags & POSIX_SPAWN_SETSID) != 0) {
    if (setsid() < 0) {
      child_fail(errfd, errno != 0 ? errno : EIO);
    }
  }
  if ((flags & POSIX_SPAWN_RESETIDS) != 0) {
    if (seteuid(getuid()) != 0 || setegid(getgid()) != 0) {
      child_fail(errfd, errno != 0 ? errno : EPERM);
    }
  }
  if ((flags & POSIX_SPAWN_SETSIGMASK) != 0) {
    if (sigprocmask(SIG_SETMASK, &attr->__ss, NULL) != 0) {
      child_fail(errfd, errno != 0 ? errno : EIO);
    }
  }
  if ((flags & POSIX_SPAWN_SETSIGDEF) != 0) {
    int sig;
    for (sig = 1; sig < DAVECC_SPAWN_NSIG; sig++) {
      if (sigismember(&attr->__sd, sig)) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = SIG_DFL;
        if (sigaction(sig, &sa, NULL) != 0) {
          child_fail(errfd, errno != 0 ? errno : EIO);
        }
      }
    }
  }
  (void)flags; /* USEVFORK accepted as no-op */
}

static void apply_file_actions(const posix_spawn_file_actions_t* fa,
                               int errfd) {
  unsigned i;
  if (fa == NULL) {
    return;
  }
  for (i = 0; i < fa->__n; i++) {
    const struct __posix_spawn_file_action* a = &fa->__acts[i];
    if (a->__type == kActClose) {
      if (close(a->__fildes) != 0 && errno != EBADF) {
        child_fail(errfd, errno != 0 ? errno : EIO);
      }
    } else if (a->__type == kActDup2) {
      if (dup2(a->__fildes, a->__arg) < 0) {
        child_fail(errfd, errno != 0 ? errno : EIO);
      }
    } else if (a->__type == kActOpen) {
      int fd;
      if (close(a->__fildes) != 0 && errno != EBADF) {
        child_fail(errfd, errno != 0 ? errno : EIO);
      }
      fd = open(a->__path, a->__arg, a->__mode);
      if (fd < 0) {
        child_fail(errfd, errno != 0 ? errno : EIO);
      }
      if (fd != a->__fildes) {
        if (dup2(fd, a->__fildes) < 0) {
          int saved = errno;
          close(fd);
          child_fail(errfd, saved != 0 ? saved : EIO);
        }
        close(fd);
      }
    } else {
      child_fail(errfd, EINVAL);
    }
  }
}

static int do_spawn(pid_t* pid, const char* path, int use_path,
                    const posix_spawn_file_actions_t* file_actions,
                    const posix_spawnattr_t* attrp, char* const argv[],
                    char* const envp[]) {
  int pfd[2];
  pid_t child;
  int err = 0;
  ssize_t n;

  if (pid == NULL || path == NULL || argv == NULL) {
    return EINVAL;
  }
  if (attrp != NULL) {
    short flags = attrp->__flags;
    if ((flags & (POSIX_SPAWN_SETSCHEDPARAM | POSIX_SPAWN_SETSCHEDULER |
                  POSIX_SPAWN_SETPGROUP)) != 0) {
      return ENOTSUP;
    }
  }

  if (pipe2(pfd, O_CLOEXEC) != 0) {
    return errno != 0 ? errno : EIO;
  }

  child = fork();
  if (child < 0) {
    err = errno != 0 ? errno : EAGAIN;
    close(pfd[0]);
    close(pfd[1]);
    return err;
  }

  if (child == 0) {
    close(pfd[0]);
    apply_attrs(attrp, pfd[1]);
    apply_file_actions(file_actions, pfd[1]);
    if (use_path) {
      extern char** environ;
      char* const* e = envp != NULL ? envp : environ;
      /* execvp only uses environ; temporarily install envp. */
      char** saved = environ;
      environ = (char**)e;
      execvp(path, (char**)argv);
      environ = saved;
    } else {
      execve(path, (char**)argv, envp != NULL ? (char**)envp : environ);
    }
    child_fail(pfd[1], errno != 0 ? errno : ENOENT);
  }

  close(pfd[1]);
  n = read(pfd[0], &err, sizeof(err));
  close(pfd[0]);
  if (n < 0) {
    err = errno != 0 ? errno : EIO;
    (void)waitpid(child, NULL, 0);
    return err;
  }
  if (n == (ssize_t)sizeof(err)) {
    int st = 0;
    (void)waitpid(child, &st, 0);
    return err != 0 ? err : EIO;
  }
  /* EOF: child closed the pipe after successful exec. */
  *pid = child;
  return 0;
}

int posix_spawn(pid_t* pid, const char* path,
                const posix_spawn_file_actions_t* file_actions,
                const posix_spawnattr_t* attrp, char* const argv[],
                char* const envp[]) {
  return do_spawn(pid, path, 0, file_actions, attrp, argv, envp);
}

int posix_spawnp(pid_t* pid, const char* file,
                 const posix_spawn_file_actions_t* file_actions,
                 const posix_spawnattr_t* attrp, char* const argv[],
                 char* const envp[]) {
  return do_spawn(pid, file, 1, file_actions, attrp, argv, envp);
}

#endif /* !__DAVECC_NATIVE_DARWIN__ */
