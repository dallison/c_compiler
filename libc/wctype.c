#include <wctype.h>

#include <ctype.h>
#include <string.h>

enum {
  class_alnum = 1ul << 0,
  class_alpha = 1ul << 1,
  class_blank = 1ul << 2,
  class_cntrl = 1ul << 3,
  class_digit = 1ul << 4,
  class_graph = 1ul << 5,
  class_lower = 1ul << 6,
  class_print = 1ul << 7,
  class_punct = 1ul << 8,
  class_space = 1ul << 9,
  class_upper = 1ul << 10,
  class_xdigit = 1ul << 11,
  transform_lower = 1ul,
  transform_upper = 2ul
};

static int narrow_value(wint_t value) {
  return value <= 0x7f ? (int)value : -1;
}

int iswalnum(wint_t value) {
  int narrow = narrow_value(value);
  return narrow >= 0 && isalnum(narrow);
}
int iswalpha(wint_t value) {
  int narrow = narrow_value(value);
  return narrow >= 0 && isalpha(narrow);
}
int iswblank(wint_t value) {
  int narrow = narrow_value(value);
  return narrow >= 0 && isblank(narrow);
}
int iswcntrl(wint_t value) {
  int narrow = narrow_value(value);
  return narrow >= 0 && iscntrl(narrow);
}
int iswdigit(wint_t value) {
  int narrow = narrow_value(value);
  return narrow >= 0 && isdigit(narrow);
}
int iswgraph(wint_t value) {
  int narrow = narrow_value(value);
  return narrow >= 0 && isgraph(narrow);
}
int iswlower(wint_t value) {
  int narrow = narrow_value(value);
  return narrow >= 0 && islower(narrow);
}
int iswprint(wint_t value) {
  int narrow = narrow_value(value);
  return narrow >= 0 && isprint(narrow);
}
int iswpunct(wint_t value) {
  int narrow = narrow_value(value);
  return narrow >= 0 && ispunct(narrow);
}
int iswspace(wint_t value) {
  int narrow = narrow_value(value);
  return narrow >= 0 && isspace(narrow);
}
int iswupper(wint_t value) {
  int narrow = narrow_value(value);
  return narrow >= 0 && isupper(narrow);
}
int iswxdigit(wint_t value) {
  int narrow = narrow_value(value);
  return narrow >= 0 && isxdigit(narrow);
}

wint_t towlower(wint_t value) {
  int narrow = narrow_value(value);
  return narrow >= 0 ? (wint_t)tolower(narrow) : value;
}

wint_t towupper(wint_t value) {
  int narrow = narrow_value(value);
  return narrow >= 0 ? (wint_t)toupper(narrow) : value;
}

wctype_t wctype(const char* property) {
  if (property == NULL) return 0;
  if (strcmp(property, "alnum") == 0) return class_alnum;
  if (strcmp(property, "alpha") == 0) return class_alpha;
  if (strcmp(property, "blank") == 0) return class_blank;
  if (strcmp(property, "cntrl") == 0) return class_cntrl;
  if (strcmp(property, "digit") == 0) return class_digit;
  if (strcmp(property, "graph") == 0) return class_graph;
  if (strcmp(property, "lower") == 0) return class_lower;
  if (strcmp(property, "print") == 0) return class_print;
  if (strcmp(property, "punct") == 0) return class_punct;
  if (strcmp(property, "space") == 0) return class_space;
  if (strcmp(property, "upper") == 0) return class_upper;
  if (strcmp(property, "xdigit") == 0) return class_xdigit;
  return 0;
}

int iswctype(wint_t value, wctype_t descriptor) {
  if ((descriptor & class_alnum) != 0 && iswalnum(value)) return 1;
  if ((descriptor & class_alpha) != 0 && iswalpha(value)) return 1;
  if ((descriptor & class_blank) != 0 && iswblank(value)) return 1;
  if ((descriptor & class_cntrl) != 0 && iswcntrl(value)) return 1;
  if ((descriptor & class_digit) != 0 && iswdigit(value)) return 1;
  if ((descriptor & class_graph) != 0 && iswgraph(value)) return 1;
  if ((descriptor & class_lower) != 0 && iswlower(value)) return 1;
  if ((descriptor & class_print) != 0 && iswprint(value)) return 1;
  if ((descriptor & class_punct) != 0 && iswpunct(value)) return 1;
  if ((descriptor & class_space) != 0 && iswspace(value)) return 1;
  if ((descriptor & class_upper) != 0 && iswupper(value)) return 1;
  if ((descriptor & class_xdigit) != 0 && iswxdigit(value)) return 1;
  return 0;
}

wctrans_t wctrans(const char* property) {
  if (property == NULL) return 0;
  if (strcmp(property, "tolower") == 0) return transform_lower;
  if (strcmp(property, "toupper") == 0) return transform_upper;
  return 0;
}

wint_t towctrans(wint_t value, wctrans_t descriptor) {
  if (descriptor == transform_lower) return towlower(value);
  if (descriptor == transform_upper) return towupper(value);
  return value;
}
