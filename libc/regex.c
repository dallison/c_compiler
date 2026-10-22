#include <regex.h>
#include <stdlib.h>
#include <string.h>

int regcomp(regex_t* preg, const char* pattern, int cflags) {
  (void)cflags;
  if (preg == NULL || pattern == NULL) return REG_BADPAT;
  preg->re_nsub = 0;
  preg->__pattern = strdup(pattern);
  return preg->__pattern == NULL ? REG_ESPACE : 0;
}

static int MatchHere(const char* pattern, const char* text);

static int MatchStar(char atom, const char* pattern, const char* text) {
  do {
    if (MatchHere(pattern, text)) return 1;
  } while (*text != '\0' && (*text++ == atom || atom == '.'));
  return 0;
}

static int MatchHere(const char* pattern, const char* text) {
  if (pattern[0] == '\0') return 1;
  if (pattern[1] == '*') {
    return MatchStar(pattern[0], pattern + 2, text);
  }
  if (pattern[0] == '$' && pattern[1] == '\0') return *text == '\0';
  if (*text != '\0' && (pattern[0] == '.' || pattern[0] == *text)) {
    return MatchHere(pattern + 1, text + 1);
  }
  return 0;
}

int regexec(const regex_t* preg, const char* string, size_t nmatch,
            regmatch_t* pmatch, int eflags) {
  (void)eflags;
  if (preg == NULL || preg->__pattern == NULL || string == NULL) return REG_BADPAT;
  const char* pattern = preg->__pattern;
  const char* start = string;
  int anchored = pattern[0] == '^';
  if (anchored) pattern++;
  for (; *start != '\0'; start++) {
    if (MatchHere(pattern, start)) {
      if (nmatch > 0 && pmatch != NULL) {
        pmatch[0].rm_so = (regoff_t)(start - string);
        const char* end = start;
        while (*end != '\0') end++;
        pmatch[0].rm_eo = (regoff_t)(end - string);
      }
      return 0;
    }
    if (anchored) break;
  }
  if (pattern[0] == '\0' || (pattern[0] == '$' && pattern[1] == '\0')) return 0;
  return REG_NOMATCH;
}

size_t regerror(int errcode, const regex_t* preg, char* errbuf, size_t errbuf_size) {
  (void)preg;
  const char* text = errcode == 0 ? "success" : "regular expression failed";
  size_t length = strlen(text);
  if (errbuf != NULL && errbuf_size != 0) {
    size_t copy = length < errbuf_size - 1 ? length : errbuf_size - 1;
    memcpy(errbuf, text, copy);
    errbuf[copy] = '\0';
  }
  return length + 1;
}

void regfree(regex_t* preg) {
  if (preg == NULL) return;
  free(preg->__pattern);
  preg->__pattern = NULL;
}
