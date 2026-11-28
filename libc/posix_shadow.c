/*
 * /etc/shadow lookups for DaveOS login / passwd.
 */
#include <errno.h>
#include <fcntl.h>
#include <shadow.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SP_LINE_MAX 256
#define SP_FIELDS 9

static int g_sp_fd = -1;
static struct spwd g_sp;
static char g_sp_buf[SP_LINE_MAX];
static char* g_sp_fields[SP_FIELDS];

static int OpenDb(const char* path) { return open(path, O_RDONLY); }

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

static long ParseLong(const char* s) {
  if (s == NULL || s[0] == '\0') {
    return -1;
  }
  return (long)atol(s);
}

static struct spwd* ParseShadowLine(char* line) {
  if (line[0] == '\0' || line[0] == '#') {
    return NULL;
  }
  size_t len = strlen(line);
  if (len >= sizeof(g_sp_buf)) {
    return NULL;
  }
  memcpy(g_sp_buf, line, len + 1);
  int n = SplitColon(g_sp_buf, g_sp_fields, SP_FIELDS);
  if (n < 2) {
    return NULL;
  }
  memset(&g_sp, 0, sizeof(g_sp));
  g_sp.sp_namp = g_sp_fields[0];
  g_sp.sp_pwdp = g_sp_fields[1];
  g_sp.sp_lstchg = n > 2 ? ParseLong(g_sp_fields[2]) : -1;
  g_sp.sp_min = n > 3 ? ParseLong(g_sp_fields[3]) : -1;
  g_sp.sp_max = n > 4 ? ParseLong(g_sp_fields[4]) : -1;
  g_sp.sp_warn = n > 5 ? ParseLong(g_sp_fields[5]) : -1;
  g_sp.sp_inact = n > 6 ? ParseLong(g_sp_fields[6]) : -1;
  g_sp.sp_expire = n > 7 ? ParseLong(g_sp_fields[7]) : -1;
  g_sp.sp_flag = n > 8 && g_sp_fields[8][0] != '\0'
                     ? (unsigned long)atol(g_sp_fields[8])
                     : 0;
  return &g_sp;
}

void setspent(void) {
  if (g_sp_fd >= 0) {
    close(g_sp_fd);
  }
  g_sp_fd = OpenDb("/etc/shadow");
}

void endspent(void) {
  if (g_sp_fd >= 0) {
    close(g_sp_fd);
    g_sp_fd = -1;
  }
}

struct spwd* getspent(void) {
  if (g_sp_fd < 0) {
    setspent();
    if (g_sp_fd < 0) {
      errno = ENOENT;
      return NULL;
    }
  }
  for (;;) {
    char line[SP_LINE_MAX];
    int n = ReadLine(g_sp_fd, line, sizeof(line));
    if (n < 0) {
      return NULL;
    }
    if (n == 0) {
      return NULL;
    }
    struct spwd* sp = ParseShadowLine(line);
    if (sp != NULL) {
      return sp;
    }
  }
}

struct spwd* getspnam(const char* name) {
  if (name == NULL) {
    errno = EINVAL;
    return NULL;
  }
  setspent();
  for (;;) {
    struct spwd* sp = getspent();
    if (sp == NULL) {
      endspent();
      return NULL;
    }
    if (strcmp(sp->sp_namp, name) == 0) {
      endspent();
      return sp;
    }
  }
}
