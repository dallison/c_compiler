/*
 * /etc/passwd and /etc/group lookups (no authentication / login yet).
 */
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PWD_LINE_MAX 256
#define PWD_FIELDS 7
#define GRP_FIELDS 4
#define GRP_MEM_MAX 8

static int OpenDb(const char* path) {
  return open(path, O_RDONLY);
}

static int ReadLine(int fd, char* buf, size_t cap) {
  size_t n = 0;
  while (n + 1 < cap) {
    char c = 0;
    ssize_t r = read(fd, &c, 1);
    if (r < 0) {
      return -1;
    }
    if (r == 0) {
      break;
    }
    if (c == '\n') {
      break;
    }
    buf[n++] = c;
  }
  buf[n] = '\0';
  return (int)n;
}

static int SplitColon(char* line, char** fields, int max_fields) {
  int n = 0;
  char* p = line;
  fields[n++] = p;
  while (*p != '\0' && n < max_fields) {
    if (*p == ':') {
      *p = '\0';
      fields[n++] = p + 1;
    }
    p++;
  }
  return n;
}

/* ---- passwd ---- */

static int g_pw_fd = -1;
static struct passwd g_pw;
static char g_pw_buf[PWD_LINE_MAX];
static char* g_pw_fields[PWD_FIELDS];

static void ClearPasswd(void) {
  memset(&g_pw, 0, sizeof(g_pw));
}

static struct passwd* ParsePasswdLine(char* line) {
  if (line[0] == '\0' || line[0] == '#') {
    return NULL;
  }
  size_t len = strlen(line);
  if (len >= sizeof(g_pw_buf)) {
    return NULL;
  }
  memcpy(g_pw_buf, line, len + 1);
  int n = SplitColon(g_pw_buf, g_pw_fields, PWD_FIELDS);
  if (n < PWD_FIELDS) {
    return NULL;
  }
  ClearPasswd();
  g_pw.pw_name = g_pw_fields[0];
  g_pw.pw_passwd = g_pw_fields[1];
  g_pw.pw_uid = (uid_t)atoi(g_pw_fields[2]);
  g_pw.pw_gid = (gid_t)atoi(g_pw_fields[3]);
  g_pw.pw_gecos = g_pw_fields[4];
  g_pw.pw_dir = g_pw_fields[5];
  g_pw.pw_shell = g_pw_fields[6];
  return &g_pw;
}

void setpwent(void) {
  if (g_pw_fd >= 0) {
    close(g_pw_fd);
  }
  g_pw_fd = OpenDb("/etc/passwd");
}

void endpwent(void) {
  if (g_pw_fd >= 0) {
    close(g_pw_fd);
    g_pw_fd = -1;
  }
}

struct passwd* getpwent(void) {
  if (g_pw_fd < 0) {
    setpwent();
    if (g_pw_fd < 0) {
      errno = ENOENT;
      return NULL;
    }
  }
  for (;;) {
    char line[PWD_LINE_MAX];
    int n = ReadLine(g_pw_fd, line, sizeof(line));
    if (n < 0) {
      return NULL;
    }
    if (n == 0) {
      return NULL;
    }
    struct passwd* pw = ParsePasswdLine(line);
    if (pw != NULL) {
      return pw;
    }
  }
}

struct passwd* getpwnam(const char* name) {
  if (name == NULL) {
    errno = EINVAL;
    return NULL;
  }
  setpwent();
  for (;;) {
    struct passwd* pw = getpwent();
    if (pw == NULL) {
      endpwent();
      return NULL;
    }
    if (strcmp(pw->pw_name, name) == 0) {
      endpwent();
      return pw;
    }
  }
}

struct passwd* getpwuid(uid_t uid) {
  setpwent();
  for (;;) {
    struct passwd* pw = getpwent();
    if (pw == NULL) {
      endpwent();
      return NULL;
    }
    if (pw->pw_uid == uid) {
      endpwent();
      return pw;
    }
  }
}

/* ---- group ---- */

static int g_gr_fd = -1;
static struct group g_gr;
static char g_gr_buf[PWD_LINE_MAX];
static char* g_gr_fields[GRP_FIELDS];
static char* g_gr_mem[GRP_MEM_MAX + 1];
static char g_gr_mem_buf[PWD_LINE_MAX];

static void ClearGroup(void) {
  memset(&g_gr, 0, sizeof(g_gr));
  memset(g_gr_mem, 0, sizeof(g_gr_mem));
}

static struct group* ParseGroupLine(char* line) {
  if (line[0] == '\0' || line[0] == '#') {
    return NULL;
  }
  size_t len = strlen(line);
  if (len >= sizeof(g_gr_buf)) {
    return NULL;
  }
  memcpy(g_gr_buf, line, len + 1);
  int n = SplitColon(g_gr_buf, g_gr_fields, GRP_FIELDS);
  if (n < 3) {
    return NULL;
  }
  ClearGroup();
  g_gr.gr_name = g_gr_fields[0];
  g_gr.gr_passwd = g_gr_fields[1];
  g_gr.gr_gid = (gid_t)atoi(g_gr_fields[2]);
  g_gr_mem[0] = NULL;
  if (n >= 4 && g_gr_fields[3] != NULL && g_gr_fields[3][0] != '\0') {
    size_t mlen = strlen(g_gr_fields[3]);
    if (mlen >= sizeof(g_gr_mem_buf)) {
      return NULL;
    }
    memcpy(g_gr_mem_buf, g_gr_fields[3], mlen + 1);
    int mi = 0;
    char* p = g_gr_mem_buf;
    g_gr_mem[mi++] = p;
    while (*p != '\0' && mi < GRP_MEM_MAX) {
      if (*p == ',') {
        *p = '\0';
        g_gr_mem[mi++] = p + 1;
      }
      p++;
    }
    g_gr_mem[mi] = NULL;
  }
  g_gr.gr_mem = g_gr_mem;
  return &g_gr;
}

void setgrent(void) {
  if (g_gr_fd >= 0) {
    close(g_gr_fd);
  }
  g_gr_fd = OpenDb("/etc/group");
}

void endgrent(void) {
  if (g_gr_fd >= 0) {
    close(g_gr_fd);
    g_gr_fd = -1;
  }
}

struct group* getgrent(void) {
  if (g_gr_fd < 0) {
    setgrent();
    if (g_gr_fd < 0) {
      errno = ENOENT;
      return NULL;
    }
  }
  for (;;) {
    char line[PWD_LINE_MAX];
    int n = ReadLine(g_gr_fd, line, sizeof(line));
    if (n < 0) {
      return NULL;
    }
    if (n == 0) {
      return NULL;
    }
    struct group* gr = ParseGroupLine(line);
    if (gr != NULL) {
      return gr;
    }
  }
}

struct group* getgrnam(const char* name) {
  if (name == NULL) {
    errno = EINVAL;
    return NULL;
  }
  setgrent();
  for (;;) {
    struct group* gr = getgrent();
    if (gr == NULL) {
      endgrent();
      return NULL;
    }
    if (strcmp(gr->gr_name, name) == 0) {
      endgrent();
      return gr;
    }
  }
}

struct group* getgrgid(gid_t gid) {
  setgrent();
  for (;;) {
    struct group* gr = getgrent();
    if (gr == NULL) {
      endgrent();
      return NULL;
    }
    if (gr->gr_gid == gid) {
      endgrent();
      return gr;
    }
  }
}
