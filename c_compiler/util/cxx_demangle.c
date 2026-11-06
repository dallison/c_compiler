//
//  cxx_demangle.c
//  c_compiler
//
//  Demangler for DaveCC's Itanium-style C++ asm names.  Mirrors the grammar
//  emitted by SymbolSetCXXMangledAsmName / AppendCXXMangledTypeName in
//  frontend/syntax/symbol.c (nested names, substitutions, operators, module
//  W…E prefixes, .nd suffixes, and a few DaveCC-specific families).
//

#include "cxx_demangle.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { kMaxSubs = 256 };

typedef struct {
  const char* p;
  const char* end;
  String* out;
  String subs[kMaxSubs];
  size_t num_subs;
  bool ok;
} Demangler;

static void DemFail(Demangler* d) {
  d->ok = false;
}

static bool DemHas(const Demangler* d, size_t n) {
  return d->ok && (size_t)(d->end - d->p) >= n;
}

static char DemPeek(const Demangler* d) {
  return DemHas(d, 1) ? d->p[0] : '\0';
}

static char DemGet(Demangler* d) {
  if (!DemHas(d, 1)) {
    DemFail(d);
    return '\0';
  }
  return *d->p++;
}

static bool DemEat(Demangler* d, char c) {
  if (DemPeek(d) != c) {
    return false;
  }
  DemGet(d);
  return true;
}

static bool DemExpect(Demangler* d, char c) {
  if (!DemEat(d, c)) {
    DemFail(d);
    return false;
  }
  return true;
}

static void DemAppend(Demangler* d, const char* s) {
  if (d->ok && s != NULL) {
    StringAppend(d->out, s);
  }
}

static void DemAppendChar(Demangler* d, char c) {
  if (d->ok) {
    StringAppendChar(d->out, c);
  }
}

static void DemAppendSegment(Demangler* d, const char* s, size_t n) {
  if (d->ok && s != NULL && n > 0) {
    StringAppendSegment(d->out, s, n);
  }
}

static bool DemParseDecimal(Demangler* d, size_t* value) {
  if (!isdigit((unsigned char)DemPeek(d))) {
    return false;
  }
  size_t v = 0;
  while (isdigit((unsigned char)DemPeek(d))) {
    v = v * 10 + (size_t)(DemGet(d) - '0');
  }
  *value = v;
  return true;
}

static void DemRemember(Demangler* d, size_t start) {
  if (!d->ok || d->num_subs >= kMaxSubs || d->out->length < start) {
    return;
  }
  StringInit(&d->subs[d->num_subs], NULL);
  StringAppendSegment(&d->subs[d->num_subs], d->out->value + start,
                      d->out->length - start);
  d->num_subs++;
}

typedef struct {
  bool is_const;
  bool is_volatile;
  bool is_lvalue_ref;
  bool is_rvalue_ref;
  bool explicit_object;
} NestedQuals;

static bool DemParseType(Demangler* d);
static bool DemParseName(Demangler* d, NestedQuals* quals);
static bool DemParseEncoding(Demangler* d);

static const char* OperatorName(const char* code, size_t len) {
  static const struct {
    const char* code;
    const char* name;
  } ops[] = {
      {"nw", "operator new"},      {"na", "operator new[]"},
      {"dl", "operator delete"},   {"da", "operator delete[]"},
      {"ps", "operator+"},         {"ng", "operator-"},
      {"ad", "operator&"},         {"de", "operator*"},
      {"co", "operator~"},         {"pl", "operator+"},
      {"mi", "operator-"},         {"ml", "operator*"},
      {"dv", "operator/"},         {"rm", "operator%"},
      {"an", "operator&"},         {"or", "operator|"},
      {"eo", "operator^"},         {"aS", "operator="},
      {"pL", "operator+="},        {"mI", "operator-="},
      {"mL", "operator*="},        {"dV", "operator/="},
      {"rM", "operator%="},        {"aN", "operator&="},
      {"oR", "operator|="},        {"eO", "operator^="},
      {"ls", "operator<<"},        {"rs", "operator>>"},
      {"lS", "operator<<="},       {"rS", "operator>>="},
      {"eq", "operator=="},        {"ne", "operator!="},
      {"lt", "operator<"},         {"gt", "operator>"},
      {"le", "operator<="},        {"ge", "operator>="},
      {"ss", "operator<=>"},       {"nt", "operator!"},
      {"aa", "operator&&"},        {"oo", "operator||"},
      {"pp", "operator++"},        {"mm", "operator--"},
      {"cm", "operator,"},         {"pm", "operator->*"},
      {"pt", "operator->"},        {"cl", "operator()"},
      {"ix", "operator[]"},        {"qu", "operator?"},
      {"cv", "operator"},          {"li", "operator\"\""},
      {"v ", NULL},
  };
  for (size_t i = 0; i < sizeof(ops) / sizeof(ops[0]); i++) {
    if (strlen(ops[i].code) == len && memcmp(ops[i].code, code, len) == 0) {
      return ops[i].name;
    }
  }
  return NULL;
}

static bool DemParseSourceName(Demangler* d) {
  size_t len = 0;
  if (!DemParseDecimal(d, &len) || !DemHas(d, len) || len == 0) {
    DemFail(d);
    return false;
  }
  const char* name = d->p;
  d->p += len;
  if (len >= 8 && memcmp(name, "operator", 8) == 0) {
    // DaveCC conversion-operator templates use "operator T" as a source name.
    DemAppendSegment(d, name, len);
    return true;
  }
  DemAppendSegment(d, name, len);
  return true;
}

static bool DemParseUnqualifiedName(Demangler* d) {
  char c = DemPeek(d);
  if (isdigit((unsigned char)c)) {
    return DemParseSourceName(d);
  }
  if (c == 'C' && DemHas(d, 2) && d->p[1] == '1') {
    d->p += 2;
    DemAppend(d, "<constructor>");
    return true;
  }
  if (c == 'D' && DemHas(d, 2) && d->p[1] == '1') {
    d->p += 2;
    DemAppend(d, "<destructor>");
    return true;
  }
  // Two-letter operator codes.
  if (DemHas(d, 2) && isalpha((unsigned char)d->p[0]) &&
      (isalnum((unsigned char)d->p[1]) || d->p[1] == ' ')) {
    const char* op = OperatorName(d->p, 2);
    if (op != NULL) {
      d->p += 2;
      DemAppend(d, op);
      return true;
    }
  }
  DemFail(d);
  return false;
}

static bool DemParseTemplateArgs(Demangler* d) {
  if (!DemEat(d, 'I')) {
    return true;
  }
  DemAppendChar(d, '<');
  bool first = true;
  while (d->ok && DemPeek(d) != 'E' && DemPeek(d) != '\0') {
    if (!first) {
      DemAppend(d, ", ");
    }
    first = false;
    size_t start = d->out->length;
    if (DemPeek(d) == 'L') {
      // Literal: L <type> <value> E
      DemGet(d);
      DemAppendChar(d, '(');
      DemParseType(d);
      DemAppendChar(d, ')');
      if (DemEat(d, 'n')) {
        DemAppendChar(d, '-');
      }
      while (isdigit((unsigned char)DemPeek(d)) ||
             (DemPeek(d) >= 'A' && DemPeek(d) <= 'Z') ||
             (DemPeek(d) >= 'a' && DemPeek(d) <= 'z')) {
        DemAppendChar(d, DemGet(d));
      }
      DemExpect(d, 'E');
    } else if (DemPeek(d) == 'X') {
      DemGet(d);
      DemAppend(d, "<expr>");
      // Skip until matching E at this level — best-effort: consume until E.
      int depth = 1;
      while (d->ok && depth > 0 && DemHas(d, 1)) {
        char ch = DemGet(d);
        if (ch == 'I' || ch == 'N' || ch == 'X' || ch == 'F') {
          depth++;
        } else if (ch == 'E') {
          depth--;
        }
      }
    } else if (DemPeek(d) == 'T') {
      DemGet(d);
      size_t idx = 0;
      DemParseDecimal(d, &idx);
      DemExpect(d, '_');
      StringPrintf(d->out, "T%zu", idx);
    } else {
      DemParseType(d);
    }
    DemRemember(d, start);
  }
  DemExpect(d, 'E');
  DemAppendChar(d, '>');
  return d->ok;
}

static bool DemParseSubstitution(Demangler* d) {
  if (!DemEat(d, 'S')) {
    return false;
  }
  // Standard abbreviations.
  char abbr = DemPeek(d);
  if (abbr == 't' || abbr == 'a' || abbr == 'b' || abbr == 's' || abbr == 'i' ||
      abbr == 'o' || abbr == 'd') {
    DemGet(d);
    switch (abbr) {
      case 't':
        DemAppend(d, "std");
        break;
      case 'a':
        DemAppend(d, "std::allocator");
        break;
      case 'b':
        DemAppend(d, "std::basic_string");
        break;
      case 's':
        DemAppend(d, "std::string");
        break;
      case 'i':
        DemAppend(d, "std::istream");
        break;
      case 'o':
        DemAppend(d, "std::ostream");
        break;
      case 'd':
        DemAppend(d, "std::iostream");
        break;
    }
    return true;
  }
  size_t index = 0;
  if (DemPeek(d) != '_') {
    // Base-36 index then '_'.
    size_t value = 0;
    bool any = false;
    while (d->ok) {
      char ch = DemPeek(d);
      size_t digit;
      if (ch >= '0' && ch <= '9') {
        digit = (size_t)(ch - '0');
      } else if (ch >= 'A' && ch <= 'Z') {
        digit = (size_t)(ch - 'A') + 10;
      } else {
        break;
      }
      DemGet(d);
      value = value * 36 + digit;
      any = true;
    }
    if (!any) {
      DemFail(d);
      return false;
    }
    index = value + 1;
  }
  DemExpect(d, '_');
  if (index >= d->num_subs) {
    DemFail(d);
    return false;
  }
  DemAppend(d, d->subs[index].value);
  return true;
}

static bool DemParseBuiltinType(Demangler* d) {
  char c = DemGet(d);
  switch (c) {
    case 'v':
      DemAppend(d, "void");
      return true;
    case 'w':
      DemAppend(d, "wchar_t");
      return true;
    case 'b':
      DemAppend(d, "bool");
      return true;
    case 'c':
      DemAppend(d, "char");
      return true;
    case 'a':
      DemAppend(d, "signed char");
      return true;
    case 'h':
      DemAppend(d, "unsigned char");
      return true;
    case 's':
      DemAppend(d, "short");
      return true;
    case 't':
      DemAppend(d, "unsigned short");
      return true;
    case 'i':
      DemAppend(d, "int");
      return true;
    case 'j':
      DemAppend(d, "unsigned int");
      return true;
    case 'l':
      DemAppend(d, "long");
      return true;
    case 'm':
      DemAppend(d, "unsigned long");
      return true;
    case 'x':
      DemAppend(d, "long long");
      return true;
    case 'y':
      DemAppend(d, "unsigned long long");
      return true;
    case 'f':
      DemAppend(d, "float");
      return true;
    case 'd':
      DemAppend(d, "double");
      return true;
    case 'e':
      DemAppend(d, "long double");
      return true;
    case 'n':
      DemAppend(d, "__int128");
      return true;
    case 'o':
      DemAppend(d, "unsigned __int128");
      return true;
    case 'z':
      DemAppend(d, "...");
      return true;
    case 'D':
      switch (DemGet(d)) {
        case 'n':
          DemAppend(d, "decltype(nullptr)");
          return true;
        case 'u':
          DemAppend(d, "char8_t");
          return true;
        case 's':
          DemAppend(d, "char16_t");
          return true;
        case 'i':
          DemAppend(d, "char32_t");
          return true;
        case 'F': {
          // DF32_ / DF64_ std::floatN_t
          size_t bits = 0;
          DemParseDecimal(d, &bits);
          DemExpect(d, '_');
          StringPrintf(d->out, "std::float%zu_t", bits);
          return true;
        }
        default:
          DemFail(d);
          return false;
      }
    case 'u': {
      // Vendor extended type: u <length> <name>
      size_t len = 0;
      if (!DemParseDecimal(d, &len) || !DemHas(d, len)) {
        DemFail(d);
        return false;
      }
      DemAppendSegment(d, d->p, len);
      d->p += len;
      return true;
    }
    default:
      DemFail(d);
      return false;
  }
}

static bool DemParseQualifiedType(Demangler* d) {
  // Leading CV / ref-qualifiers: K V R O P
  // Collect then print after base type for readability (int const*).
  char quals[16];
  size_t nq = 0;
  while (d->ok) {
    char c = DemPeek(d);
    if (c == 'K' || c == 'V' || c == 'R' || c == 'O' || c == 'P') {
      if (nq < sizeof(quals)) {
        quals[nq++] = DemGet(d);
      } else {
        DemGet(d);
      }
    } else {
      break;
    }
  }
  size_t start = d->out->length;
  if (DemPeek(d) == 'S') {
    DemParseSubstitution(d);
  } else if (DemPeek(d) == 'N' || isdigit((unsigned char)DemPeek(d)) ||
             DemPeek(d) == 'Z') {
    DemParseName(d, NULL);
  } else if (DemPeek(d) == 'A') {
    DemGet(d);
    size_t dim = 0;
    DemParseDecimal(d, &dim);
    DemExpect(d, '_');
    DemParseType(d);
    StringPrintf(d->out, "[%zu]", dim);
  } else if (DemPeek(d) == 'F') {
    DemGet(d);
    DemAppend(d, "(");
    DemParseType(d);  // return
    DemAppend(d, " (");
    bool first = true;
    while (d->ok && DemPeek(d) != 'E') {
      if (!first) {
        DemAppend(d, ", ");
      }
      first = false;
      DemParseType(d);
    }
    DemExpect(d, 'E');
    DemAppend(d, "))");
  } else if (DemPeek(d) == 'M') {
    DemGet(d);
    DemParseType(d);
    DemAppend(d, "::*");
    DemParseType(d);
  } else {
    DemParseBuiltinType(d);
  }
  // Apply qualifiers in reverse (outermost first in mangling).
  for (size_t i = nq; i > 0; i--) {
    switch (quals[i - 1]) {
      case 'P':
        DemAppendChar(d, '*');
        break;
      case 'R':
        DemAppendChar(d, '&');
        break;
      case 'O':
        DemAppend(d, "&&");
        break;
      case 'K':
        DemAppend(d, " const");
        break;
      case 'V':
        DemAppend(d, " volatile");
        break;
    }
  }
  (void)start;
  return d->ok;
}

static bool DemParseType(Demangler* d) {
  size_t start = d->out->length;
  bool ok = DemParseQualifiedType(d);
  if (ok) {
    DemParseTemplateArgs(d);
    DemRemember(d, start);
  }
  return ok;
}

static void DemAppendNestedQuals(Demangler* d, const NestedQuals* q) {
  if (q == NULL) {
    return;
  }
  if (q->explicit_object) {
    DemAppend(d, " /*explicit object*/");
  }
  if (q->is_const) {
    DemAppend(d, " const");
  }
  if (q->is_volatile) {
    DemAppend(d, " volatile");
  }
  if (q->is_lvalue_ref) {
    DemAppend(d, " &");
  }
  if (q->is_rvalue_ref) {
    DemAppend(d, " &&");
  }
}

static bool DemParseNestedName(Demangler* d, NestedQuals* quals) {
  if (!DemExpect(d, 'N')) {
    return false;
  }
  NestedQuals local = {0};
  for (;;) {
    char c = DemPeek(d);
    if (c == 'K') {
      DemGet(d);
      local.is_const = true;
    } else if (c == 'V') {
      DemGet(d);
      local.is_volatile = true;
    } else if (c == 'R') {
      DemGet(d);
      local.is_lvalue_ref = true;
    } else if (c == 'O') {
      DemGet(d);
      local.is_rvalue_ref = true;
    } else if (c == 'H') {
      DemGet(d);
      local.explicit_object = true;
    } else {
      break;
    }
  }
  size_t start = d->out->length;
  bool first = true;
  while (d->ok && DemPeek(d) != 'E' && DemPeek(d) != '\0') {
    if (!first) {
      DemAppend(d, "::");
    }
    first = false;
    if (DemPeek(d) == 'S') {
      DemParseSubstitution(d);
    } else if (isdigit((unsigned char)DemPeek(d)) || DemPeek(d) == 'C' ||
               DemPeek(d) == 'D' || isalpha((unsigned char)DemPeek(d))) {
      DemParseUnqualifiedName(d);
      DemParseTemplateArgs(d);
    } else {
      DemFail(d);
      break;
    }
    DemRemember(d, start);
  }
  DemExpect(d, 'E');
  if (quals != NULL) {
    *quals = local;
  } else {
    DemAppendNestedQuals(d, &local);
  }
  return d->ok;
}

static bool DemParseLocalName(Demangler* d) {
  // Z <function encoding> E <entity> [<discriminator>]
  if (!DemExpect(d, 'Z')) {
    return false;
  }
  size_t scope_start = d->out->length;
  DemParseEncoding(d);
  DemExpect(d, 'E');
  String scope;
  StringInit(&scope, NULL);
  if (d->out->length > scope_start) {
    StringAppendSegment(&scope, d->out->value + scope_start,
                        d->out->length - scope_start);
    StringErase(d->out, scope_start, d->out->length - scope_start);
  }
  if (DemPeek(d) == 'N') {
    DemParseNestedName(d, NULL);
  } else {
    DemParseUnqualifiedName(d);
    DemParseTemplateArgs(d);
  }
  if (DemEat(d, '_')) {
    if (DemEat(d, '_')) {
      size_t n = 0;
      DemParseDecimal(d, &n);
      DemExpect(d, '_');
      StringPrintf(d->out, " (local #%zu)", n + 1);
    } else if (isdigit((unsigned char)DemPeek(d))) {
      size_t n = 0;
      DemParseDecimal(d, &n);
      StringPrintf(d->out, " (local #%zu)", n + 1);
    }
  }
  if (scope.length > 0) {
    String prefixed;
    StringInit(&prefixed, scope.value);
    StringAppend(&prefixed, "::");
    StringAppendSegment(&prefixed, d->out->value + scope_start,
                        d->out->length - scope_start);
    StringErase(d->out, scope_start, d->out->length - scope_start);
    StringAppend(d->out, prefixed.value);
    StringDestruct(&prefixed);
  }
  StringDestruct(&scope);
  return d->ok;
}

static bool DemParseName(Demangler* d, NestedQuals* quals) {
  // Nested-name components register substitutions themselves.  A bare
  // unqualified encoding name (e.g. `pl` in `_Zpl3BoxS_`) is not a
  // substitution candidate — only types and nested prefixes are.
  if (DemPeek(d) == 'N') {
    DemParseNestedName(d, quals);
  } else if (DemPeek(d) == 'Z') {
    DemParseLocalName(d);
  } else if (DemPeek(d) == 'S') {
    DemParseSubstitution(d);
  } else {
    DemParseUnqualifiedName(d);
    DemParseTemplateArgs(d);
  }
  return d->ok;
}

static bool DemParseBareFunctionType(Demangler* d) {
  DemAppendChar(d, '(');
  // Lone 'v' means no parameters.
  if (DemPeek(d) == 'v' &&
      (d->p + 1 == d->end || d->p[1] == '.' || d->p[1] == 'E')) {
    DemGet(d);
    DemAppendChar(d, ')');
    return true;
  }
  bool first = true;
  while (d->ok && DemPeek(d) != '\0' && DemPeek(d) != '.' &&
         DemPeek(d) != 'E') {
    const char* save = d->p;
    size_t out_len = d->out->length;
    size_t subs = d->num_subs;
    if (!first) {
      DemAppend(d, ", ");
    }
    if (!DemParseType(d)) {
      d->p = save;
      StringErase(d->out, out_len, d->out->length - out_len);
      d->num_subs = subs;
      d->ok = true;
      break;
    }
    first = false;
  }
  DemAppendChar(d, ')');
  return d->ok;
}

static bool DemParseEncoding(Demangler* d) {
  NestedQuals quals = {0};
  DemParseName(d, &quals);
  // Function parameter types (absent for data symbols).
  if (d->ok && DemPeek(d) != '\0' && DemPeek(d) != '.' && DemPeek(d) != 'E') {
    DemParseBareFunctionType(d);
  }
  DemAppendNestedQuals(d, &quals);
  // DaveCC non-deduced suffix .nd[dn]*
  if (DemEat(d, '.')) {
    if (DemEat(d, 'n') && DemEat(d, 'd')) {
      while (DemPeek(d) == 'd' || DemPeek(d) == 'n') {
        DemGet(d);
      }
      DemAppend(d, " [non-deduced]");
    } else {
      DemFail(d);
    }
  }
  return d->ok;
}

static void DemInit(Demangler* d, const char* mangled, String* out) {
  memset(d, 0, sizeof(*d));
  d->p = mangled;
  d->end = mangled + strlen(mangled);
  d->out = out;
  d->ok = true;
}

static void DemDestruct(Demangler* d) {
  for (size_t i = 0; i < d->num_subs; i++) {
    StringDestruct(&d->subs[i]);
  }
}

static bool DemParseModulePrefix(Demangler* d) {
  // W <len><name>... E
  if (!DemEat(d, 'W')) {
    return true;
  }
  bool first = true;
  DemAppend(d, "");  // module path printed before rest with trailing ::
  String module;
  StringInit(&module, NULL);
  while (d->ok && DemPeek(d) != 'E' && DemPeek(d) != '\0') {
    size_t len = 0;
    if (!DemParseDecimal(d, &len) || !DemHas(d, len) || len == 0) {
      DemFail(d);
      break;
    }
    if (!first) {
      StringAppend(&module, ".");
    }
    first = false;
    StringAppendSegment(&module, d->p, len);
    d->p += len;
  }
  DemExpect(d, 'E');
  if (module.length > 0) {
    StringAppend(d->out, module.value);
    StringAppend(d->out, "::");
  }
  StringDestruct(&module);
  return d->ok;
}

bool CXXIsMangledName(const char* name) {
  if (name == NULL) {
    return false;
  }
  if (strncmp(name, "_Z", 2) == 0) {
    return true;
  }
  if (strncmp(name, "__Z", 3) == 0) {
    return true;
  }
  if (strncmp(name, "__davecc_vtbl_", 14) == 0) {
    return true;
  }
  return false;
}

static bool DemangleTypeBody(const char* body, String* out) {
  Demangler d;
  DemInit(&d, body, out);
  bool ok = DemParseType(&d) && d.p == d.end && d.ok;
  DemDestruct(&d);
  return ok;
}

static bool DemangleSpecial(const char* mangled, String* out) {
  if (strncmp(mangled, "_ZTS", 4) == 0) {
    StringAppend(out, "typeinfo name for ");
    String body;
    StringInit(&body, NULL);
    if (!DemangleTypeBody(mangled + 4, &body)) {
      StringDestruct(&body);
      StringClear(out);
      return false;
    }
    StringAppendString(out, &body);
    StringDestruct(&body);
    return true;
  }
  if (strncmp(mangled, "_ZTI", 4) == 0) {
    StringAppend(out, "typeinfo for ");
    String body;
    StringInit(&body, NULL);
    if (!DemangleTypeBody(mangled + 4, &body)) {
      StringDestruct(&body);
      StringClear(out);
      return false;
    }
    StringAppendString(out, &body);
    StringDestruct(&body);
    return true;
  }
  if (strncmp(mangled, "_ZTA", 4) == 0) {
    StringAppend(out, "template parameter object");
    return true;
  }
  if (strncmp(mangled, "__davecc_vtbl_", 14) == 0) {
    StringAppend(out, "vtable for ");
    StringAppend(out, mangled + 14);
    return true;
  }
  return false;
}

bool CXXDemangle(const char* mangled, String* out) {
  if (mangled == NULL || out == NULL) {
    return false;
  }
  StringClear(out);
  if (DemangleSpecial(mangled, out)) {
    return out->length > 0;
  }
  const char* p = mangled;
  if (p[0] == '_' && p[1] == '_' && p[2] == 'Z') {
    p += 2;  // __Z → treat as _Z from second underscore? Actually __Z means
             // leading _ + _Z on Mach-O. Skip one underscore.
    p = mangled + 1;
  }
  if (p[0] != '_' || p[1] != 'Z') {
    return false;
  }
  p += 2;

  Demangler d;
  DemInit(&d, p, out);
  DemParseModulePrefix(&d);
  DemParseEncoding(&d);
  bool ok = d.ok && d.p == d.end && out->length > 0;
  DemDestruct(&d);
  if (!ok) {
    StringClear(out);
  }
  return ok;
}

const char* CXXFormatSymbol(const char* name, bool demangle, char* buf,
                            size_t buf_size) {
  if (name == NULL) {
    return "";
  }
  if (!demangle || buf == NULL || buf_size == 0 || !CXXIsMangledName(name)) {
    return name;
  }
  String demangled;
  StringInit(&demangled, NULL);
  if (CXXDemangle(name, &demangled) && demangled.length > 0 &&
      demangled.length + 1 <= buf_size) {
    memcpy(buf, demangled.value, demangled.length + 1);
    StringDestruct(&demangled);
    return buf;
  }
  StringDestruct(&demangled);
  return name;
}

// StringErase / StringClear may not exist — check dstring API.
