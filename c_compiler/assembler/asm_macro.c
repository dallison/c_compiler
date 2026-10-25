//
// GNU as compatible macros for the shared assembler.
//
// Definitions are recorded as text and expanded by substitution, then pushed
// back as a nested source so every target assembler sees ordinary statements.
// .rept repeats text.  .irp / .irpc substitute one formal per iteration, using
// the same replacement rules as .macro.  Conditional directives are evaluated
// while the expansion is assembled, which is what lets a macro recurse and
// then stop (the gas `sum` example).
//

#include "asm_macro.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "source.h"

#define ASM_MAX_MACRO_DEPTH 256
#define ASM_MAX_REPEAT 1000000

typedef enum {
  kFormalOptional,
  kFormalRequired,
  kFormalVararg,
} FormalKind;

typedef struct Formal {
  String name;
  String def;
  String actual;
  FormalKind kind;
} Formal;

typedef struct AsmMacro {
  String name;
  Vector formals;
  String body;
  int count;
} AsmMacro;

typedef struct ExpansionFrame {
  bool saved_suppress;
  String pending;
} ExpansionFrame;

typedef struct IncludeFrame {
  Source* source;
  bool saved_suppress;
} IncludeFrame;

typedef struct CondFrame {
  bool parent_active;
  bool taken;
  bool seen_else;
  bool active;
} CondFrame;

typedef enum {
  kCmpNonZero,
  kCmpEq,
  kCmpNe,
  kCmpGt,
  kCmpGe,
  kCmpLt,
  kCmpLe,
} CondCompare;

static bool Is6502(const Assembler* assembler) {
  return assembler->object.elf_machine_type == ELF_MACHINE_TYPEW65C02;
}

static char LowerChar(char ch) {
  return (char)tolower((unsigned char)ch);
}

static bool NameEqualIgnoreCase(const char* a, const char* b) {
  while (*a != '\0' && *b != '\0') {
    if (LowerChar(*a) != LowerChar(*b)) {
      return false;
    }
    a++;
    b++;
  }
  return *a == '\0' && *b == '\0';
}

static void LowercaseString(String* text) {
  for (size_t i = 0; i < text->length; i++) {
    text->value[i] = LowerChar(text->value[i]);
  }
}

static bool NameBegin(char ch) {
  return isalpha((unsigned char)ch) || ch == '_' || ch == '.' || ch == '@';
}

static bool NamePart(char ch) {
  return NameBegin(ch) || isdigit((unsigned char)ch) || ch == '$';
}

static bool IgnorableLine(const String* line) {
  return line->length == 0 || (line->length == 1 && line->value[0] == '\0');
}

static size_t SkipWhite(const char* in, size_t len, size_t i) {
  while (i < len && (in[i] == ' ' || in[i] == '\t' || in[i] == '\r')) {
    i++;
  }
  return i;
}

static bool AtComment(const char* in, size_t len, size_t i) {
  return i + 1 < len && in[i] == '/' && (in[i + 1] == '/' || in[i + 1] == '*');
}

static bool AtStatementEnd(const Assembler* assembler, const char* in,
                           size_t len, size_t i) {
  if (i >= len || AtComment(in, len, i)) {
    return true;
  }
  return in[i] == ';' && !Is6502(assembler);
}

static size_t SkipComma(const char* in, size_t len, size_t i) {
  i = SkipWhite(in, len, i);
  if (i < len && in[i] == ',') {
    i = SkipWhite(in, len, i + 1);
  }
  return i;
}

static void FreeFormal(void* value) {
  Formal* formal = value;
  StringDestruct(&formal->name);
  StringDestruct(&formal->def);
  StringDestruct(&formal->actual);
}

static void FreeMacro(AsmMacro* macro) {
  if (macro == NULL) {
    return;
  }
  StringDestruct(&macro->name);
  StringDestruct(&macro->body);
  VectorDestructWithContents(&macro->formals, FreeFormal, true);
  free(macro);
}

static void FreeMacroKV(MapKeyValue* kv, void* data) {
  (void)data;
  FreeMacro(kv->value.p);
}

static Formal* FindFormal(Vector* formals, const char* name) {
  if (name == NULL || name[0] == '\0') {
    return NULL;
  }
  for (size_t i = 0; i < formals->length; i++) {
    Formal* formal = formals->value.p[i];
    if (strcmp(formal->name.value, name) == 0) {
      return formal;
    }
  }
  return NULL;
}

static AsmMacro* FindMacro(Assembler* assembler, const char* spelling) {
  String name;
  StringInit(&name, spelling);
  LowercaseString(&name);
  AsmMacro* macro = MapFindPointerKey(&assembler->macros, name.value);
  StringDestruct(&name);
  return macro;
}

static bool StatementSeparator(const Assembler* assembler) {
  return !Is6502(assembler);
}

static void FinishDirectiveLine(Assembler* assembler) {
  if (assembler->lex.current_token == TOK(semicolon) &&
      StatementSeparator(assembler)) {
    return;
  }
  size_t i = assembler->lex.line.length;
  if (assembler->lex.current_token != TOK(eof) && !LexEof(&assembler->lex)) {
    i = assembler->lex.token_start;
  }
  bool quote = false;
  for (; i < assembler->lex.line.length; i++) {
    char ch = assembler->lex.line.value[i];
    if (ch == '"') {
      quote = !quote;
      continue;
    }
    if (quote) {
      continue;
    }
    if (ch == ';' && StatementSeparator(assembler)) {
      assembler->lex.pos = i;
      LexNextToken(&assembler->lex);
      return;
    }
    if (AtComment(assembler->lex.line.value, assembler->lex.line.length, i)) {
      break;
    }
  }
  assembler->lex.pos = assembler->lex.line.length;
  assembler->lex.current_token = TOK(eof);
}

void AssemblerSkipInactiveStatement(Assembler* assembler) {
  size_t i = assembler->lex.pos;
  bool quote = false;
  for (; i < assembler->lex.line.length; i++) {
    char ch = assembler->lex.line.value[i];
    if (ch == '"') {
      quote = !quote;
      continue;
    }
    if (quote) {
      continue;
    }
    if (ch == ';' && StatementSeparator(assembler)) {
      assembler->lex.pos = i;
      LexNextToken(&assembler->lex);
      return;
    }
    if (AtComment(assembler->lex.line.value, assembler->lex.line.length, i)) {
      break;
    }
  }
  assembler->lex.pos = assembler->lex.line.length;
  assembler->lex.current_token = TOK(eof);
}

static CondFrame* CondTop(const Assembler* assembler) {
  if (assembler->cond_stack.length == 0) {
    return NULL;
  }
  return assembler->cond_stack.value.p[assembler->cond_stack.length - 1];
}

bool AssemblerCondActive(const Assembler* assembler) {
  CondFrame* top = CondTop(assembler);
  return top == NULL || top->active;
}

static bool CondParentActive(const Assembler* assembler) {
  CondFrame* top = CondTop(assembler);
  return top == NULL || top->active;
}

static void CondPush(Assembler* assembler, bool condition) {
  if (assembler->cond_stack.length >= ASM_MAX_MACRO_DEPTH) {
    AssemblerError(assembler, "conditional nesting is too deep");
    return;
  }
  CondFrame* frame = calloc(1, sizeof(CondFrame));
  frame->parent_active = CondParentActive(assembler);
  frame->taken = frame->parent_active && condition;
  frame->active = frame->taken;
  VectorAppend(&assembler->cond_stack, frame);
}

static void ClearCondStack(Assembler* assembler) {
  VectorClearWithContents(&assembler->cond_stack, NULL, true);
}

bool AssemblerIsConditionalDirective(const char* spelling) {
  static const char* kNames[] = {
      ".if",    ".ifdef", ".ifndef", ".ifb",   ".ifnb",  ".ifc",   ".ifnc",
      ".ifeq",  ".ifne",  ".ifgt",   ".ifge",  ".iflt",  ".ifle",  ".ifeqs",
      ".ifnes", ".else",  ".elseif", ".endif", ".ifnotdef", NULL,
  };
  for (size_t i = 0; kNames[i] != NULL; i++) {
    if (NameEqualIgnoreCase(spelling, kNames[i])) {
      return true;
    }
  }
  return false;
}

static ExpansionFrame* PopExpansion(Assembler* assembler) {
  if (assembler->expansion_frames.length == 0) {
    return NULL;
  }
  ExpansionFrame* frame =
      assembler->expansion_frames
          .value.p[assembler->expansion_frames.length - 1];
  VectorPop(&assembler->expansion_frames);
  Source* dead = assembler->lex.source;
  if (dead != NULL && dead->prev != NULL) {
    assembler->lex.source = dead->prev;
    dead->prev = NULL;
    SourceDelete(dead);
  }
  assembler->lex.suppress_preprocessing = frame->saved_suppress;
  return frame;
}

static void DiscardExpansionFrame(ExpansionFrame* frame) {
  if (frame == NULL) {
    return;
  }
  StringDestruct(&frame->pending);
  free(frame);
}

static bool PopFinishedInclude(Assembler* assembler) {
  if (assembler->include_frames.length == 0) {
    return false;
  }
  if (assembler->lex.source == NULL || !SourceEof(assembler->lex.source) ||
      !IgnorableLine(&assembler->lex.line)) {
    return false;
  }
  IncludeFrame* frame =
      assembler->include_frames.value.p[assembler->include_frames.length - 1];
  if (assembler->lex.source != frame->source) {
    return false;
  }
  Source* dead = assembler->lex.source;
  assembler->lex.source = dead->prev;
  dead->prev = NULL;
  SourceDelete(dead);
  assembler->lex.suppress_preprocessing = frame->saved_suppress;
  VectorPop(&assembler->include_frames);
  free(frame);
  return true;
}

static void PopIncludeNow(Assembler* assembler) {
  IncludeFrame* frame =
      assembler->include_frames.value.p[assembler->include_frames.length - 1];
  VectorPop(&assembler->include_frames);
  Source* dead = frame->source;
  if (assembler->lex.source == dead) {
    assembler->lex.source = dead->prev;
    assembler->lex.suppress_preprocessing = frame->saved_suppress;
  } else {
    Source** link = &assembler->lex.source;
    while (*link != NULL && *link != dead) {
      link = &(*link)->prev;
    }
    if (*link == dead) {
      *link = dead->prev;
    }
  }
  dead->prev = NULL;
  SourceDelete(dead);
  free(frame);
}

static void UnwindNestedSources(Assembler* assembler) {
  while (assembler->include_frames.length > 0 ||
         assembler->expansion_frames.length > 0) {
    if (assembler->include_frames.length > 0) {
      IncludeFrame* frame = assembler->include_frames
                                .value.p[assembler->include_frames.length - 1];
      if (assembler->lex.source == frame->source) {
        PopIncludeNow(assembler);
        continue;
      }
    }
    if (assembler->expansion_frames.length > 0) {
      DiscardExpansionFrame(PopExpansion(assembler));
      continue;
    }
    if (assembler->include_frames.length > 0) {
      PopIncludeNow(assembler);
    }
  }
}

void AssemblerReadAssemblyLine(Assembler* assembler) {
  for (;;) {
    LexReadLine(&assembler->lex);
    if (PopFinishedInclude(assembler)) {
      continue;
    }
    if (assembler->expansion_frames.length == 0) {
      return;
    }
    if (!SourceEof(assembler->lex.source) ||
        !IgnorableLine(&assembler->lex.line)) {
      return;
    }
    ExpansionFrame* frame = PopExpansion(assembler);
    if (frame != NULL && frame->pending.length > 0) {
      LexWillChangeLine(&assembler->lex);
      StringSetString(&assembler->lex.line, &frame->pending);
      assembler->lex.pos = 0;
      DiscardExpansionFrame(frame);
      return;
    }
    DiscardExpansionFrame(frame);
  }
}

static bool InputExhausted(Assembler* assembler) {
  return IgnorableLine(&assembler->lex.line) &&
         SourceEof(assembler->lex.source) &&
         assembler->expansion_frames.length == 0;
}

static void PushExpansion(Assembler* assembler, String* text, String* pending) {
  Source* source = NewSourceFromString("<macro-expansion>", text);
  source->prev = assembler->lex.source;
  assembler->lex.source = source;
  ExpansionFrame* frame = calloc(1, sizeof(ExpansionFrame));
  frame->saved_suppress = assembler->lex.suppress_preprocessing;
  StringInit(&frame->pending, NULL);
  if (pending != NULL && pending->length > 0) {
    StringSetString(&frame->pending, pending);
  }
  assembler->lex.suppress_preprocessing = true;
  VectorAppend(&assembler->expansion_frames, frame);
  assembler->lex.pos = assembler->lex.line.length;
  assembler->lex.current_token = TOK(eof);
}

static size_t ReadName(const Assembler* assembler, const char* in, size_t len,
                       size_t idx, String* name) {
  StringClear(name);
  if (idx >= len || !NameBegin(in[idx])) {
    return idx;
  }
  StringAppendChar(name, in[idx++]);
  while (idx < len && NamePart(in[idx])) {
    StringAppendChar(name, in[idx++]);
  }
  if (assembler->altmacro && idx < len && in[idx] == '&') {
    idx++;
  }
  return idx;
}

static bool PercentExpr(Assembler* assembler, const char* in, size_t len,
                        size_t idx, size_t* next, int64_t* value) {
  String saved_line;
  String saved_spelling;
  String saved_literal;
  String saved_suffix;
  StringInitFromSegment(&saved_line, assembler->lex.line.value,
                        assembler->lex.line.length);
  StringInitFromSegment(&saved_spelling, assembler->lex.spelling.value,
                        assembler->lex.spelling.length);
  StringInitFromSegment(&saved_literal, assembler->lex.literal_spelling.value,
                        assembler->lex.literal_spelling.length);
  StringInitFromSegment(&saved_suffix, assembler->lex.suffix.value,
                        assembler->lex.suffix.length);
  size_t saved_pos = assembler->lex.pos;
  Token saved_token = assembler->lex.current_token;
  SourceLocation saved_loc = assembler->lex.current_token_location;
  int64_t saved_number = assembler->lex.number;
  double saved_fnumber = assembler->lex.fnumber;
  size_t saved_token_start = assembler->lex.token_start;
  int saved_token_lineno = assembler->lex.token_lineno;
  bool saved_dollar = assembler->lex.dollar_prefixed;
  size_t saved_index = 0;
  int saved_lineno = assembler->lex.source->lineno;
  bool saved_reached = assembler->lex.source->reached_eof;
  if (assembler->lex.source->device == kSourceFromString) {
    saved_index = assembler->lex.source->from.string.index;
  }

  LexWillChangeLine(&assembler->lex);
  StringClear(&assembler->lex.line);
  if (idx + 1 < len) {
    StringAppendSegment(&assembler->lex.line, in + idx + 1, len - (idx + 1));
  }
  assembler->lex.pos = 0;
  LexNextToken(&assembler->lex);
  bool known = false;
  int64_t parsed = AssemblerEvaluateKnownExpression(assembler, &known);
  if (assembler->lex.current_token == TOK(eof) || LexEof(&assembler->lex)) {
    *next = len;
  } else {
    *next = idx + 1 + assembler->lex.token_start;
    if (*next > len) {
      *next = len;
    }
  }

  LexWillChangeLine(&assembler->lex);
  StringSetString(&assembler->lex.line, &saved_line);
  assembler->lex.pos = saved_pos;
  assembler->lex.current_token = saved_token;
  assembler->lex.current_token_location = saved_loc;
  StringSetString(&assembler->lex.spelling, &saved_spelling);
  StringSetString(&assembler->lex.literal_spelling, &saved_literal);
  StringSetString(&assembler->lex.suffix, &saved_suffix);
  assembler->lex.number = saved_number;
  assembler->lex.fnumber = saved_fnumber;
  assembler->lex.token_start = saved_token_start;
  assembler->lex.token_lineno = saved_token_lineno;
  assembler->lex.dollar_prefixed = saved_dollar;
  assembler->lex.source->lineno = saved_lineno;
  assembler->lex.source->reached_eof = saved_reached;
  if (assembler->lex.source->device == kSourceFromString) {
    assembler->lex.source->from.string.index = saved_index;
  }
  StringDestruct(&saved_line);
  StringDestruct(&saved_spelling);
  StringDestruct(&saved_literal);
  StringDestruct(&saved_suffix);
  if (!known) {
    AssemblerError(assembler, "%% operator needs an absolute expression");
    *value = 0;
    return false;
  }
  *value = parsed;
  return true;
}

// Quoted, bracketed, or bare macro argument, following gas get_any_string.
static size_t GetAnyString(Assembler* assembler, const char* in, size_t len,
                           size_t idx, String* out) {
  StringClear(out);
  idx = SkipWhite(in, len, idx);
  if (idx >= len || AtStatementEnd(assembler, in, len, idx) || in[idx] == ',') {
    return idx;
  }
  if (assembler->altmacro && in[idx] == '%') {
    size_t next = idx;
    int64_t value = 0;
    PercentExpr(assembler, in, len, idx, &next, &value);
    StringAppendInt64(out, value);
    return next;
  }
  if (in[idx] == '"' ||
      (assembler->altmacro && (in[idx] == '<' || in[idx] == '\''))) {
    bool wrap = assembler->altmacro && in[idx] != '<';
    if (wrap) {
      StringAppendChar(out, '"');
    }
    while (idx < len &&
           (in[idx] == '"' ||
            (assembler->altmacro && (in[idx] == '<' || in[idx] == '\'')))) {
      if (in[idx] == '<') {
        int nest = 0;
        idx++;
        while (idx < len && (in[idx] != '>' || nest > 0)) {
          if (in[idx] == '!') {
            idx++;
            if (idx < len) {
              StringAppendChar(out, in[idx++]);
            }
            continue;
          }
          if (in[idx] == '<') {
            nest++;
          } else if (in[idx] == '>') {
            nest--;
          }
          StringAppendChar(out, in[idx++]);
        }
        if (idx < len && in[idx] == '>') {
          idx++;
        }
      } else {
        char quote = in[idx++];
        bool escaped = false;
        while (idx < len) {
          if (idx > 0 && in[idx - 1] == '\\') {
            escaped = !escaped;
          } else {
            escaped = false;
          }
          if (assembler->altmacro && in[idx] == '!') {
            idx++;
            if (idx < len) {
              StringAppendChar(out, in[idx++]);
            }
            continue;
          }
          if (escaped && in[idx] == quote) {
            StringAppendChar(out, quote);
            idx++;
            continue;
          }
          if (in[idx] == quote) {
            idx++;
            if (idx >= len || in[idx] != quote) {
              break;
            }
          }
          StringAppendChar(out, in[idx++]);
        }
      }
    }
    if (wrap) {
      StringAppendChar(out, '"');
    }
    return idx;
  }

  int paren = 0;
  int bracket = 0;
  while (idx < len &&
         (paren > 0 || bracket > 0 ||
          (in[idx] != ' ' && in[idx] != '\t' && in[idx] != ',' &&
           in[idx] != ';' && !AtComment(in, len, idx))) &&
         (in[idx] != '<' || !assembler->altmacro)) {
    char ch = in[idx];
    if (ch == '"' || ch == '\'') {
      StringAppendChar(out, in[idx++]);
      while (idx < len && in[idx] != ch) {
        StringAppendChar(out, in[idx++]);
      }
      if (idx < len) {
        StringAppendChar(out, in[idx++]);
      }
      continue;
    }
    if (ch == '(') {
      paren++;
    } else if (ch == ')' && paren > 0) {
      paren--;
    } else if (ch == '[') {
      bracket++;
    } else if (ch == ']' && bracket > 0) {
      bracket--;
    }
    StringAppendChar(out, ch);
    idx++;
  }
  return idx;
}

static bool ParseFormals(Assembler* assembler, const char* in, size_t len,
                         size_t* idx, AsmMacro* macro) {
  size_t i = SkipWhite(in, len, *idx);
  while (i < len && !AtStatementEnd(assembler, in, len, i)) {
    Formal* formal = calloc(1, sizeof(Formal));
    StringInit(&formal->name, NULL);
    StringInit(&formal->def, NULL);
    StringInit(&formal->actual, NULL);
    formal->kind = kFormalOptional;
    i = ReadName(assembler, in, len, i, &formal->name);
    if (formal->name.length == 0) {
      AssemblerError(assembler, "Bad parameter list for macro `%s'",
                     macro->name.value);
      FreeFormal(formal);
      free(formal);
      return false;
    }
    i = SkipWhite(in, len, i);
    // ':' starts a qualifier when it is not itself a name character.  The
    // following token is `req` or `vararg`, so the next character is a name
    // character and must not reject the qualifier.
    if (i < len && in[i] == ':' &&
        (!NameBegin(':') || i + 1 >= len || !NamePart(in[i + 1]))) {
      String qual;
      StringInit(&qual, NULL);
      i = ReadName(assembler, in, len, SkipWhite(in, len, i + 1), &qual);
      if (strcmp(qual.value, "req") == 0) {
        formal->kind = kFormalRequired;
      } else if (strcmp(qual.value, "vararg") == 0) {
        formal->kind = kFormalVararg;
      } else {
        AssemblerError(assembler,
                       "`%s' is not a valid parameter qualifier for `%s' in "
                       "macro `%s'",
                       qual.value, formal->name.value, macro->name.value);
        StringDestruct(&qual);
        FreeFormal(formal);
        free(formal);
        return false;
      }
      StringDestruct(&qual);
      i = SkipWhite(in, len, i);
    }
    if (i < len && in[i] == '=') {
      i = GetAnyString(assembler, in, len, i + 1, &formal->def);
      i = SkipWhite(in, len, i);
      if (formal->kind == kFormalRequired) {
        AssemblerWarning(assembler, "macro",
                         "Pointless default value for required parameter `%s' "
                         "in macro `%s'",
                         formal->name.value, macro->name.value);
        StringClear(&formal->def);
      }
    }
    if (FindFormal(&macro->formals, formal->name.value) != NULL) {
      AssemblerError(assembler,
                     "A parameter named `%s' already exists for macro `%s'",
                     formal->name.value, macro->name.value);
      FreeFormal(formal);
      free(formal);
      return false;
    }
    VectorAppend(&macro->formals, formal);
    if (formal->kind == kFormalVararg) {
      break;
    }
    size_t before = i;
    i = SkipComma(in, len, i);
    if (i == before) {
      break;
    }
  }
  i = SkipWhite(in, len, i);
  if (i < len && !AtStatementEnd(assembler, in, len, i)) {
    AssemblerError(assembler, "Bad parameter list for macro `%s'",
                   macro->name.value);
    return false;
  }
  *idx = i;
  return true;
}

static bool FirstDirective(const Assembler* assembler, const char* line,
                           size_t len, char* name, size_t name_cap) {
  size_t i = 0;
  bool quote = false;
  while (i < len) {
    if (!quote && AtComment(line, len, i)) {
      return false;
    }
    if (line[i] == '"') {
      quote = !quote;
    }
    if (!quote && (line[i] == ' ' || line[i] == '\t')) {
      i++;
      continue;
    }
    break;
  }
  if (i >= len || quote) {
    return false;
  }
  for (;;) {
    if (i >= len || !NameBegin(line[i])) {
      return false;
    }
    size_t start = i;
    i++;
    while (i < len && NamePart(line[i])) {
      i++;
    }
    size_t end = i;
    size_t after = SkipWhite(line, len, i);
    if (after < len && line[after] == ':') {
      i = SkipWhite(line, len, after + 1);
      continue;
    }
    size_t n = end - start;
    if (n + 1 > name_cap) {
      n = name_cap - 1;
    }
    for (size_t k = 0; k < n; k++) {
      name[k] = LowerChar(line[start + k]);
    }
    name[n] = '\0';
    (void)assembler;
    return true;
  }
}

static bool DirectiveIs(const char* name, const char* spelling) {
  return strcmp(name, spelling) == 0;
}

static bool IsRepeatOpen(const char* name) {
  return DirectiveIs(name, ".rept") || DirectiveIs(name, ".irp") ||
         DirectiveIs(name, ".irpc") || DirectiveIs(name, ".rep") ||
         DirectiveIs(name, ".irep") || DirectiveIs(name, ".irepc");
}

static bool ReadNestedBody(Assembler* assembler, bool repeat, String* body) {
  int depth = 1;
  StringClear(body);
  for (;;) {
    AssemblerReadAssemblyLine(assembler);
    if (InputExhausted(assembler)) {
      return false;
    }
    char directive[64];
    bool have = FirstDirective(assembler, assembler->lex.line.value,
                               assembler->lex.line.length, directive,
                               sizeof(directive));
    if (have) {
      bool open = repeat ? IsRepeatOpen(directive)
                         : DirectiveIs(directive, ".macro");
      bool close = repeat ? DirectiveIs(directive, ".endr")
                          : DirectiveIs(directive, ".endm");
      if (open) {
        depth++;
      }
      if (close) {
        depth--;
        if (depth == 0) {
          return true;
        }
      }
    }
    StringAppendString(body, &assembler->lex.line);
    StringAppendChar(body, '\n');
  }
}

static size_t SubActual(const Assembler* assembler, const char* in, size_t len,
                        size_t idx, int kind, bool copy_if_missing,
                        Vector* formals, Vector* locals, String* out) {
  String name;
  StringInit(&name, NULL);
  size_t start = idx;
  idx = ReadName(assembler, in, len, idx, &name);
  if (kind == '&' && idx < len && in[idx] == '&') {
    idx++;
  }
  Formal* formal = FindFormal(locals, name.value);
  if (formal == NULL) {
    formal = FindFormal(formals, name.value);
  }
  if (formal != NULL) {
    if (formal->actual.length > 0) {
      StringAppendString(out, &formal->actual);
    } else {
      StringAppendString(out, &formal->def);
    }
  } else if (kind == '&') {
    StringAppendChar(out, '&');
    StringAppendString(out, &name);
    if (idx != start && in[idx - 1] == '&') {
      StringAppendChar(out, '&');
    }
  } else if (copy_if_missing) {
    StringAppendString(out, &name);
  } else {
    StringAppendChar(out, '\\');
    StringAppendString(out, &name);
  }
  StringDestruct(&name);
  return idx;
}

static void AppendInt(String* out, int value) {
  StringAppendInt64(out, value);
}

// Substitutes formals, \@, \+, \(), and (in .altmacro) bare names, `&`, and
// LOCAL.  `plus` is used only for a real macro invocation.
static void ExpandBody(Assembler* assembler, const char* in, size_t len,
                       Vector* formals, bool is_macro, bool have_plus,
                       int plus, String* out) {
  Vector locals;
  VectorInit(&locals);
  bool inquote = false;
  size_t src = 0;
  while (src < len) {
    if (in[src] == '&') {
      src = SubActual(assembler, in, len, src + 1, '&', false, formals,
                      &locals, out);
      continue;
    }
    if (in[src] == '\\') {
      src++;
      if (src < len && in[src] == '(') {
        src++;
        while (src < len && in[src] != ')') {
          StringAppendChar(out, in[src++]);
        }
        if (src < len) {
          src++;
        } else {
          AssemblerError(assembler, "missing `)'");
        }
        continue;
      }
      if (src < len && in[src] == '@') {
        src++;
        AppendInt(out, assembler->macro_invocation_count);
        continue;
      }
      if (src < len && in[src] == '+' && have_plus) {
        src++;
        AppendInt(out, plus);
        continue;
      }
      if (src < len && in[src] == '&') {
        StringAppendChar(out, '\\');
        StringAppendChar(out, '&');
        src++;
        continue;
      }
      src = SubActual(assembler, in, len, src, '\'', false, formals, &locals,
                      out);
      continue;
    }
    if (assembler->altmacro && NameBegin(in[src])) {
      bool local_directive =
          is_macro && !inquote && src + 5 < len &&
          strncasecmp(in + src, "LOCAL", 5) == 0 &&
          isspace((unsigned char)in[src + 5]);
      if (!local_directive) {
        src = SubActual(assembler, in, len, src, '\'', true, formals, &locals,
                        out);
        continue;
      }
      src = SkipWhite(in, len, src + 5);
      while (src < len && in[src] != '\n') {
        Formal* local = calloc(1, sizeof(Formal));
        StringInit(&local->name, NULL);
        StringInit(&local->def, NULL);
        StringInit(&local->actual, NULL);
        src = ReadName(assembler, in, len, src, &local->name);
        if (local->name.length == 0) {
          FreeFormal(local);
          free(local);
          break;
        }
        if (FindFormal(&locals, local->name.value) != NULL ||
            FindFormal(formals, local->name.value) != NULL) {
          AssemblerError(assembler,
                         "`%s' was already used as parameter (or another local) "
                         "name",
                         local->name.value);
          FreeFormal(local);
          free(local);
        } else {
          char buf[32];
          snprintf(buf, sizeof(buf), ".LL%04x",
                   ++assembler->macro_local_count);
          StringSet(&local->actual, buf);
          VectorAppend(&locals, local);
        }
        src = SkipComma(in, len, src);
      }
      continue;
    }
    if (in[src] == '"') {
      inquote = !inquote;
    }
    StringAppendChar(out, in[src++]);
  }
  VectorDestructWithContents(&locals, FreeFormal, true);
  if (out->length == 0 || out->value[out->length - 1] != '\n') {
    StringAppendChar(out, '\n');
  }
}

static bool BindArguments(Assembler* assembler, AsmMacro* macro, const char* in,
                          size_t len, size_t idx, size_t* end) {
  for (size_t i = 0; i < macro->formals.length; i++) {
    Formal* formal = macro->formals.value.p[i];
    StringClear(&formal->actual);
  }
  bool saw_keyword = false;
  size_t positional = 0;
  idx = SkipWhite(in, len, idx);
  while (idx < len && !AtStatementEnd(assembler, in, len, idx)) {
    size_t scan = idx;
    while (scan < len && in[scan] != '=' && in[scan] != ' ' &&
           in[scan] != '\t' && in[scan] != ',' && in[scan] != ';' &&
           in[scan] != '"' && in[scan] != '\'' && in[scan] != '(' &&
           in[scan] != ')') {
      scan++;
    }
    if (!assembler->altmacro && scan < len && scan > idx && in[scan] == '=') {
      saw_keyword = true;
      String name;
      StringInit(&name, NULL);
      idx = ReadName(assembler, in, len, idx, &name);
      if (idx >= len || in[idx] != '=') {
        AssemblerError(assembler, "confusion in formal parameters");
        StringDestruct(&name);
        return false;
      }
      Formal* formal = FindFormal(&macro->formals, name.value);
      if (formal == NULL) {
        AssemblerError(assembler,
                       "Parameter named `%s' does not exist for macro `%s'",
                       name.value, macro->name.value);
        String discarded;
        StringInit(&discarded, NULL);
        idx = GetAnyString(assembler, in, len, idx + 1, &discarded);
        StringDestruct(&discarded);
      } else {
        if (formal->actual.length > 0) {
          AssemblerWarning(assembler, "macro",
                           "Value for parameter `%s' of macro `%s' was already "
                           "specified",
                           formal->name.value, macro->name.value);
          StringClear(&formal->actual);
        }
        idx = GetAnyString(assembler, in, len, idx + 1, &formal->actual);
      }
      StringDestruct(&name);
    } else {
      if (saw_keyword) {
        AssemblerError(assembler, "can't mix positional and keyword arguments");
        return false;
      }
      if (positional >= macro->formals.length) {
        AssemblerError(assembler, "too many positional arguments");
        return false;
      }
      Formal* formal = macro->formals.value.p[positional];
      if (formal->kind != kFormalVararg) {
        idx = GetAnyString(assembler, in, len, idx, &formal->actual);
      } else if (idx < len && in[idx] != ';' &&
                 !AtComment(in, len, idx)) {
        size_t stop = idx;
        while (stop < len && in[stop] != ';' && !AtComment(in, len, stop)) {
          stop++;
        }
        StringAppendSegment(&formal->actual, in + idx, stop - idx);
        idx = stop;
      }
      positional++;
    }
    size_t before = idx;
    idx = SkipComma(in, len, idx);
    if (idx == before) {
      break;
    }
  }
  for (size_t i = 0; i < macro->formals.length; i++) {
    Formal* formal = macro->formals.value.p[i];
    if (formal->kind == kFormalRequired && formal->actual.length == 0) {
      AssemblerError(assembler,
                     "Missing value for required parameter `%s' of macro `%s'",
                     formal->name.value, macro->name.value);
      return false;
    }
  }
  *end = idx;
  return true;
}

static void InvokeMacro(Assembler* assembler, AsmMacro* macro, size_t arg_at) {
  if (assembler->expansion_frames.length >= ASM_MAX_MACRO_DEPTH) {
    AssemblerError(assembler, "macro nesting is too deep");
    assembler->lex.pos = assembler->lex.line.length;
    assembler->lex.current_token = TOK(eof);
    return;
  }
  // Copy the line: %expr evaluation reuses the lexer line buffer.
  String line;
  StringInitFromSegment(&line, assembler->lex.line.value,
                        assembler->lex.line.length);
  size_t end = arg_at;
  if (!BindArguments(assembler, macro, line.value, line.length, arg_at, &end)) {
    StringDestruct(&line);
    assembler->lex.pos = assembler->lex.line.length;
    assembler->lex.current_token = TOK(eof);
    return;
  }
  String pending;
  StringInit(&pending, NULL);
  if (end < line.length && !AtComment(line.value, line.length, end)) {
    StringAppendSegment(&pending, line.value + end, line.length - end);
  }
  StringDestruct(&line);
  String* expanded = malloc(sizeof(String));
  StringInit(expanded, NULL);
  int plus = macro->count;
  ExpandBody(assembler, macro->body.value, macro->body.length, &macro->formals,
             true, true, plus, expanded);
  macro->count++;
  assembler->macro_invocation_count++;
  PushExpansion(assembler, expanded, &pending);
  StringDestruct(&pending);
}

bool AssemblerMacroTryInvoke(Assembler* assembler, const char* spelling) {
  if (!AssemblerCondActive(assembler)) {
    return false;
  }
  AsmMacro* macro = FindMacro(assembler, spelling);
  if (macro == NULL) {
    return false;
  }
  size_t start = assembler->lex.line.length;
  if (assembler->lex.current_token != TOK(eof) && !LexEof(&assembler->lex)) {
    start = assembler->lex.token_start;
  }
  InvokeMacro(assembler, macro, start);
  return true;
}

static AsmMacro* StartMacro(Assembler* assembler, const char* spelling) {
  AsmMacro* macro = calloc(1, sizeof(AsmMacro));
  StringInit(&macro->name, spelling);
  LowercaseString(&macro->name);
  VectorInit(&macro->formals);
  StringInit(&macro->body, NULL);
  macro->count = 0;
  if (FindMacro(assembler, macro->name.value) != NULL) {
    AssemblerError(assembler, "Macro `%s' was already defined",
                   macro->name.value);
    FreeMacro(macro);
    return NULL;
  }
  return macro;
}

void HandleDirective_macro(Assembler* assembler) {
  if (!AssemblerCondActive(assembler)) {
    AssemblerSkipInactiveStatement(assembler);
    return;
  }
  AsmMacro* macro = NULL;
  bool ok = true;
  if (!AssemblerLookingAtSymbol(assembler)) {
    AssemblerError(assembler, "Missing macro name");
    ok = false;
  } else {
    macro = StartMacro(assembler, assembler->lex.spelling.value);
    if (macro == NULL) {
      ok = false;
    } else {
      String line;
      StringInitFromSegment(&line, assembler->lex.line.value,
                            assembler->lex.line.length);
      size_t idx = assembler->lex.pos;
      if (!ParseFormals(assembler, line.value, line.length, &idx, macro)) {
        ok = false;
      }
      StringDestruct(&line);
    }
  }
  String body;
  StringInit(&body, NULL);
  if (!ReadNestedBody(assembler, false, &body)) {
    AssemblerError(assembler, "unexpected end of file in macro `%s' definition",
                   macro != NULL ? macro->name.value : "");
    ok = false;
  }
  if (!ok || macro == NULL) {
    StringDestruct(&body);
    FreeMacro(macro);
    return;
  }
  if (FindMacro(assembler, macro->name.value) != NULL) {
    AssemblerError(assembler, "Macro `%s' was already defined",
                   macro->name.value);
    StringDestruct(&body);
    FreeMacro(macro);
    return;
  }
  StringSetString(&macro->body, &body);
  StringDestruct(&body);
  MapKeyValue kv;
  kv.key.p = macro->name.value;
  kv.value.p = macro;
  MapInsert(&assembler->macros, kv);
}

void HandleDirective_endm(Assembler* assembler) {
  if (!AssemblerCondActive(assembler)) {
    return;
  }
  AssemblerError(assembler, "`.endm' without `.macro'");
  FinishDirectiveLine(assembler);
}

void HandleDirective_exitm(Assembler* assembler) {
  if (!AssemblerCondActive(assembler)) {
    AssemblerSkipInactiveStatement(assembler);
    return;
  }
  if (assembler->expansion_frames.length == 0) {
    AssemblerError(assembler, ".exitm outside of a macro");
    FinishDirectiveLine(assembler);
    return;
  }
  ExpansionFrame* frame = PopExpansion(assembler);
  if (frame != NULL && frame->pending.length > 0) {
    LexWillChangeLine(&assembler->lex);
    StringSetString(&assembler->lex.line, &frame->pending);
    assembler->lex.pos = 0;
    assembler->replay_current_line = true;
  } else {
    assembler->lex.pos = assembler->lex.line.length;
    assembler->lex.current_token = TOK(eof);
  }
  DiscardExpansionFrame(frame);
}

void HandleDirective_purgem(Assembler* assembler) {
  if (!AssemblerCondActive(assembler)) {
    AssemblerSkipInactiveStatement(assembler);
    return;
  }
  if (!AssemblerLookingAtSymbol(assembler)) {
    AssemblerError(assembler, "Missing macro name");
    FinishDirectiveLine(assembler);
    return;
  }
  String name;
  StringInit(&name, assembler->lex.spelling.value);
  LowercaseString(&name);
  LexNextToken(&assembler->lex);
  MapKeyType key;
  key.p = name.value;
  AsmMacro* macro = MapRemove(&assembler->macros, key);
  if (macro == NULL) {
    AssemblerWarning(assembler, "macro",
                     "Attempt to purge non-existing macro `%s'", name.value);
  } else {
    FreeMacro(macro);
  }
  StringDestruct(&name);
  FinishDirectiveLine(assembler);
}

static void ExpandRepeat(Assembler* assembler, int64_t count, String* body,
                          String* pending) {
  if (count < 0 || count > ASM_MAX_REPEAT) {
    AssemblerError(assembler, "repeat count %" PRId64 " is out of range",
                   count);
    return;
  }
  if (count == 0 || body->length == 0) {
    if (pending != NULL && pending->length > 0) {
      LexWillChangeLine(&assembler->lex);
      StringSetString(&assembler->lex.line, pending);
      assembler->lex.pos = 0;
      assembler->replay_current_line = true;
    }
    return;
  }
  if (assembler->expansion_frames.length >= ASM_MAX_MACRO_DEPTH) {
    AssemblerError(assembler, "macro nesting is too deep");
    return;
  }
  String* text = malloc(sizeof(String));
  StringInit(text, NULL);
  for (int64_t i = 0; i < count; i++) {
    StringAppendString(text, body);
  }
  PushExpansion(assembler, text, pending);
}

static void CaptureLineTail(Assembler* assembler, String* pending) {
  StringClear(pending);
  if (assembler->lex.current_token == TOK(eof) || LexEof(&assembler->lex)) {
    return;
  }
  size_t at = assembler->lex.token_start;
  if (at >= assembler->lex.line.length ||
      AtComment(assembler->lex.line.value, assembler->lex.line.length, at)) {
    return;
  }
  StringAppendSegment(pending, assembler->lex.line.value + at,
                      assembler->lex.line.length - at);
}

void HandleDirective_rept(Assembler* assembler) {
  if (!AssemblerCondActive(assembler)) {
    AssemblerSkipInactiveStatement(assembler);
    return;
  }
  int64_t count = AssemblerEvaluateExpression(assembler);
  String pending;
  StringInit(&pending, NULL);
  CaptureLineTail(assembler, &pending);
  String body;
  StringInit(&body, NULL);
  if (!ReadNestedBody(assembler, true, &body)) {
    AssemblerError(assembler, "unexpected end of file in `.rept'");
    StringDestruct(&body);
    StringDestruct(&pending);
    return;
  }
  ExpandRepeat(assembler, count, &body, &pending);
  StringDestruct(&body);
  StringDestruct(&pending);
}

static void ReplayOrPush(Assembler* assembler, String* text, String* pending) {
  if (text->length == 0) {
    if (pending != NULL && pending->length > 0) {
      LexWillChangeLine(&assembler->lex);
      StringSetString(&assembler->lex.line, pending);
      assembler->lex.pos = 0;
      assembler->replay_current_line = true;
    }
    StringDestruct(text);
    free(text);
    return;
  }
  PushExpansion(assembler, text, pending);
}

static void ExpandIrp(Assembler* assembler, const char* symbol, bool irpc,
                      const char* values, size_t values_len, String* body) {
  if (assembler->expansion_frames.length >= ASM_MAX_MACRO_DEPTH) {
    AssemblerError(assembler, "macro nesting is too deep");
    return;
  }
  Formal formal;
  StringInit(&formal.name, symbol);
  StringInit(&formal.def, NULL);
  StringInit(&formal.actual, NULL);
  formal.kind = kFormalOptional;
  Vector formals;
  VectorInit(&formals);
  VectorAppend(&formals, &formal);

  String* text = malloc(sizeof(String));
  StringInit(text, NULL);
  size_t idx = SkipComma(values, values_len, 0);
  if (idx >= values_len || AtStatementEnd(assembler, values, values_len, idx)) {
    StringClear(&formal.actual);
    ExpandBody(assembler, body->value, body->length, &formals, false, false, 0,
               text);
  } else if (!irpc) {
    while (idx < values_len &&
           !AtStatementEnd(assembler, values, values_len, idx)) {
      StringClear(&formal.actual);
      idx = GetAnyString(assembler, values, values_len, idx, &formal.actual);
      String chunk;
      StringInit(&chunk, NULL);
      ExpandBody(assembler, body->value, body->length, &formals, false, false,
                 0, &chunk);
      StringAppendString(text, &chunk);
      StringDestruct(&chunk);
      size_t before = idx;
      idx = SkipComma(values, values_len, idx);
      if (idx == before) {
        break;
      }
    }
  } else {
    bool in_quotes = false;
    if (values[idx] == '"') {
      in_quotes = true;
      idx++;
    }
    while (idx < values_len &&
           (in_quotes ||
            !AtStatementEnd(assembler, values, values_len, idx))) {
      if (values[idx] == '"') {
        size_t next = SkipWhite(values, values_len, idx + 1);
        if (next >= values_len ||
            AtStatementEnd(assembler, values, values_len, next)) {
          break;
        }
        in_quotes = !in_quotes;
      }
      StringClear(&formal.actual);
      StringAppendChar(&formal.actual, values[idx]);
      idx++;
      String chunk;
      StringInit(&chunk, NULL);
      ExpandBody(assembler, body->value, body->length, &formals, false, false,
                 0, &chunk);
      StringAppendString(text, &chunk);
      StringDestruct(&chunk);
      if (!in_quotes) {
        idx = SkipWhite(values, values_len, idx);
      }
    }
  }
  VectorDestruct(&formals);
  StringDestruct(&formal.name);
  StringDestruct(&formal.def);
  StringDestruct(&formal.actual);
  String pending;
  StringInit(&pending, NULL);
  if (idx < values_len && !AtComment(values, values_len, idx)) {
    StringAppendSegment(&pending, values + idx, values_len - idx);
  }
  ReplayOrPush(assembler, text, &pending);
  StringDestruct(&pending);
}

static void HandleIrp(Assembler* assembler, bool irpc) {
  if (!AssemblerCondActive(assembler)) {
    AssemblerSkipInactiveStatement(assembler);
    return;
  }
  if (!AssemblerLookingAtSymbol(assembler)) {
    AssemblerError(assembler, "missing model parameter");
    FinishDirectiveLine(assembler);
    return;
  }
  String symbol;
  StringInit(&symbol, assembler->lex.spelling.value);
  size_t value_at = assembler->lex.pos;
  String values;
  StringInitFromSegment(&values, assembler->lex.line.value + value_at,
                        assembler->lex.line.length - value_at);
  String body;
  StringInit(&body, NULL);
  if (!ReadNestedBody(assembler, true, &body)) {
    AssemblerError(assembler, "unexpected end of file in irp or irpc");
    StringDestruct(&symbol);
    StringDestruct(&values);
    StringDestruct(&body);
    return;
  }
  ExpandIrp(assembler, symbol.value, irpc, values.value, values.length, &body);
  StringDestruct(&symbol);
  StringDestruct(&values);
  StringDestruct(&body);
}

void HandleDirective_irp(Assembler* assembler) { HandleIrp(assembler, false); }

void HandleDirective_irpc(Assembler* assembler) { HandleIrp(assembler, true); }

void HandleDirective_endr(Assembler* assembler) {
  if (!AssemblerCondActive(assembler)) {
    return;
  }
  AssemblerError(assembler, "`.endr' without `.rept', `.irp', or `.irpc'");
  FinishDirectiveLine(assembler);
}

void HandleDirective_altmacro(Assembler* assembler) {
  if (!AssemblerCondActive(assembler)) {
    AssemblerSkipInactiveStatement(assembler);
    return;
  }
  assembler->altmacro = true;
  FinishDirectiveLine(assembler);
}

void HandleDirective_noaltmacro(Assembler* assembler) {
  if (!AssemblerCondActive(assembler)) {
    AssemblerSkipInactiveStatement(assembler);
    return;
  }
  assembler->altmacro = false;
  FinishDirectiveLine(assembler);
}

static void CondOnExpression(Assembler* assembler, CondCompare compare) {
  bool parent = CondParentActive(assembler);
  bool condition = false;
  if (parent) {
    int64_t value = AssemblerEvaluateExpression(assembler);
    switch (compare) {
      case kCmpNonZero:
        condition = value != 0;
        break;
      case kCmpEq:
        condition = value == 0;
        break;
      case kCmpNe:
        condition = value != 0;
        break;
      case kCmpGt:
        condition = value > 0;
        break;
      case kCmpGe:
        condition = value >= 0;
        break;
      case kCmpLt:
        condition = value < 0;
        break;
      case kCmpLe:
        condition = value <= 0;
        break;
    }
  }
  FinishDirectiveLine(assembler);
  CondPush(assembler, condition);
}

void HandleDirective_if(Assembler* assembler) {
  CondOnExpression(assembler, kCmpNonZero);
}

void HandleDirective_ifeq(Assembler* assembler) {
  CondOnExpression(assembler, kCmpEq);
}

void HandleDirective_ifne(Assembler* assembler) {
  CondOnExpression(assembler, kCmpNe);
}

void HandleDirective_ifgt(Assembler* assembler) {
  CondOnExpression(assembler, kCmpGt);
}

void HandleDirective_ifge(Assembler* assembler) {
  CondOnExpression(assembler, kCmpGe);
}

void HandleDirective_iflt(Assembler* assembler) {
  CondOnExpression(assembler, kCmpLt);
}

void HandleDirective_ifle(Assembler* assembler) {
  CondOnExpression(assembler, kCmpLe);
}

static void CondOnDefined(Assembler* assembler, bool want_defined) {
  bool parent = CondParentActive(assembler);
  bool condition = false;
  if (parent) {
    if (!AssemblerLookingAtSymbol(assembler)) {
      AssemblerError(assembler, "symbol expected");
    } else {
      AssemblerSymbol* symbol =
          AssemblerFindSymbol(assembler, assembler->lex.spelling.value);
      bool defined = symbol != NULL && symbol->defined &&
                     symbol->defined_pass == assembler->object.pass;
      condition = want_defined ? defined : !defined;
      LexNextToken(&assembler->lex);
    }
  }
  FinishDirectiveLine(assembler);
  CondPush(assembler, condition);
}

void HandleDirective_ifdef(Assembler* assembler) {
  CondOnDefined(assembler, true);
}

void HandleDirective_ifndef(Assembler* assembler) {
  CondOnDefined(assembler, false);
}

static size_t RawArgumentStart(Assembler* assembler) {
  if (assembler->lex.current_token == TOK(eof) || LexEof(&assembler->lex)) {
    return assembler->lex.line.length;
  }
  return assembler->lex.token_start;
}

static void CondOnBlank(Assembler* assembler, bool want_blank) {
  bool parent = CondParentActive(assembler);
  bool blank = true;
  if (parent) {
    String line;
    String arg;
    StringInitFromSegment(&line, assembler->lex.line.value,
                          assembler->lex.line.length);
    StringInit(&arg, NULL);
    GetAnyString(assembler, line.value, line.length, RawArgumentStart(assembler),
                 &arg);
    blank = arg.length == 0;
    StringDestruct(&arg);
    StringDestruct(&line);
  }
  FinishDirectiveLine(assembler);
  CondPush(assembler, want_blank ? blank : !blank);
}

void HandleDirective_ifb(Assembler* assembler) {
  CondOnBlank(assembler, true);
}

void HandleDirective_ifnb(Assembler* assembler) {
  CondOnBlank(assembler, false);
}

static void CondOnStrings(Assembler* assembler, bool want_equal) {
  bool parent = CondParentActive(assembler);
  bool equal = false;
  if (parent) {
    String line;
    String left;
    String right;
    StringInitFromSegment(&line, assembler->lex.line.value,
                          assembler->lex.line.length);
    StringInit(&left, NULL);
    StringInit(&right, NULL);
    size_t idx = RawArgumentStart(assembler);
    idx = GetAnyString(assembler, line.value, line.length, idx, &left);
    idx = SkipComma(line.value, line.length, idx);
    GetAnyString(assembler, line.value, line.length, idx, &right);
    equal = strcmp(left.value != NULL ? left.value : "",
                   right.value != NULL ? right.value : "") == 0;
    StringDestruct(&left);
    StringDestruct(&right);
    StringDestruct(&line);
  }
  FinishDirectiveLine(assembler);
  CondPush(assembler, want_equal ? equal : !equal);
}

void HandleDirective_ifc(Assembler* assembler) {
  CondOnStrings(assembler, true);
}

void HandleDirective_ifnc(Assembler* assembler) {
  CondOnStrings(assembler, false);
}

void HandleDirective_ifeqs(Assembler* assembler) {
  CondOnStrings(assembler, true);
}

void HandleDirective_ifnes(Assembler* assembler) {
  CondOnStrings(assembler, false);
}

void HandleDirective_else(Assembler* assembler) {
  CondFrame* frame = CondTop(assembler);
  if (frame == NULL) {
    AssemblerError(assembler, "`.else' without `.if'");
    FinishDirectiveLine(assembler);
    return;
  }
  if (frame->seen_else) {
    AssemblerError(assembler, "duplicate `.else'");
    FinishDirectiveLine(assembler);
    return;
  }
  frame->seen_else = true;
  frame->active = frame->parent_active && !frame->taken;
  FinishDirectiveLine(assembler);
}

void HandleDirective_elseif(Assembler* assembler) {
  CondFrame* frame = CondTop(assembler);
  if (frame == NULL) {
    AssemblerError(assembler, "`.elseif' without `.if'");
    FinishDirectiveLine(assembler);
    return;
  }
  if (frame->seen_else) {
    AssemblerError(assembler, "`.elseif' after `.else'");
    FinishDirectiveLine(assembler);
    return;
  }
  bool condition = false;
  if (frame->parent_active && !frame->taken) {
    condition = AssemblerEvaluateExpression(assembler) != 0;
  }
  if (condition) {
    frame->taken = true;
    frame->active = true;
  } else {
    frame->active = false;
  }
  FinishDirectiveLine(assembler);
}

void HandleDirective_endif(Assembler* assembler) {
  if (assembler->cond_stack.length == 0) {
    AssemblerError(assembler, "`.endif' without `.if'");
    FinishDirectiveLine(assembler);
    return;
  }
  CondFrame* frame =
      assembler->cond_stack.value.p[assembler->cond_stack.length - 1];
  VectorPop(&assembler->cond_stack);
  free(frame);
  FinishDirectiveLine(assembler);
}

void HandleDirective_ifnotdef(Assembler* assembler) {
  HandleDirective_ifndef(assembler);
}

static bool OpenAssemblyFile(const char* path, FILE** out) {
  FILE* fp = fopen(path, "rb");
  if (fp == NULL) {
    return false;
  }
  struct stat st;
  if (fstat(fileno(fp), &st) == 0 && S_ISDIR(st.st_mode)) {
    fclose(fp);
    errno = EISDIR;
    return false;
  }
  *out = fp;
  return true;
}

static bool JoinPath(char* dest, size_t dest_len, const char* dir,
                     const char* name) {
  int written = snprintf(dest, dest_len, "%s/%s", dir, name);
  return written > 0 && (size_t)written < dest_len;
}

static FILE* FindAssemblyFile(Assembler* assembler, const char* name,
                              char* full, size_t full_len) {
  FILE* fp = NULL;
  if (name[0] == '/') {
    if (OpenAssemblyFile(name, &fp)) {
      snprintf(full, full_len, "%s", name);
    }
    return fp;
  }
  Source* source = assembler->lex.source;
  while (source != NULL &&
         (source->filename.length == 0 || source->filename.value[0] == '<')) {
    source = source->prev;
  }
  if (source != NULL) {
    const char* slash = strrchr(source->filename.value, '/');
    if (slash != NULL) {
      char dir[4096];
      size_t dir_len = (size_t)(slash - source->filename.value);
      if (dir_len < sizeof(dir)) {
        memcpy(dir, source->filename.value, dir_len);
        dir[dir_len] = '\0';
        if (JoinPath(full, full_len, dir, name) &&
            OpenAssemblyFile(full, &fp)) {
          return fp;
        }
      }
    }
  }
  if (OpenAssemblyFile(name, &fp)) {
    snprintf(full, full_len, "%s", name);
    return fp;
  }
  Vector* paths = &assembler->preprocessor.user_include_paths;
  for (size_t i = 0; i < paths->length; i++) {
    String* dir = paths->value.p[i];
    if (JoinPath(full, full_len, dir->value, name) &&
        OpenAssemblyFile(full, &fp)) {
      return fp;
    }
  }
  return NULL;
}

static void PushIncludeSource(Assembler* assembler, const char* path, FILE* fp) {
  Source* source = NewSourceFromFile(path, fp);
  source->prev = assembler->lex.source;
  IncludeFrame* frame = calloc(1, sizeof(IncludeFrame));
  frame->source = source;
  frame->saved_suppress = assembler->lex.suppress_preprocessing;
  assembler->lex.source = source;
  assembler->lex.suppress_preprocessing = true;
  VectorAppend(&assembler->include_frames, frame);
}

void HandleDirective_include(Assembler* assembler) {
  if (!LexLookingAt(&assembler->lex, TOK(string))) {
    AssemblerError(assembler, ".include needs a quoted file name");
    return;
  }
  String name;
  StringInit(&name, assembler->lex.spelling.value);
  LexNextToken(&assembler->lex);
  char full[4096];
  FILE* fp = FindAssemblyFile(assembler, name.value, full, sizeof(full));
  if (fp == NULL) {
    AssemblerError(assembler, "cannot open include file `%s'", name.value);
    StringDestruct(&name);
    return;
  }
  PushIncludeSource(assembler, full, fp);
  StringDestruct(&name);
}

void HandleDirective_incbin(Assembler* assembler) {
  if (!LexLookingAt(&assembler->lex, TOK(string))) {
    AssemblerError(assembler, ".incbin needs a quoted file name");
    return;
  }
  String name;
  StringInit(&name, assembler->lex.spelling.value);
  LexNextToken(&assembler->lex);
  int64_t skip = 0;
  int64_t count = -1;
  if (LexMatch(&assembler->lex, TOK(comma))) {
    skip = AssemblerEvaluateExpression(assembler);
    if (LexMatch(&assembler->lex, TOK(comma))) {
      count = AssemblerEvaluateExpression(assembler);
    }
  }
  if (skip < 0 || count < -1) {
    AssemblerError(assembler, "negative .incbin operand");
    StringDestruct(&name);
    return;
  }
  char full[4096];
  FILE* fp = FindAssemblyFile(assembler, name.value, full, sizeof(full));
  if (fp == NULL) {
    AssemblerError(assembler, "cannot open binary file `%s'", name.value);
    StringDestruct(&name);
    return;
  }
  if (fseek(fp, 0, SEEK_END) != 0) {
    AssemblerError(assembler, "cannot read `%s'", name.value);
    fclose(fp);
    StringDestruct(&name);
    return;
  }
  long file_size = ftell(fp);
  if (file_size < 0 || skip > file_size ||
      fseek(fp, (long)skip, SEEK_SET) != 0) {
    AssemblerError(assembler, ".incbin skip past the end of `%s'", name.value);
    fclose(fp);
    StringDestruct(&name);
    return;
  }
  int64_t read = 0;
  int ch;
  while ((ch = fgetc(fp)) != EOF) {
    if (count >= 0 && read >= count) {
      break;
    }
    AssemblerEmitByte(assembler, assembler->object.current_section,
                      (uint8_t)ch);
    read++;
  }
  if (ferror(fp)) {
    AssemblerError(assembler, "error reading `%s'", name.value);
  }
  fclose(fp);
  StringDestruct(&name);
}

void AssemblerMacroInit(Assembler* assembler) {
  MapInitForCharPointerKeys(&assembler->macros);
  assembler->altmacro = false;
  assembler->macro_invocation_count = 0;
  assembler->macro_local_count = 0;
  VectorInit(&assembler->expansion_frames);
  VectorInit(&assembler->cond_stack);
  VectorInit(&assembler->include_frames);
  VectorInit(&assembler->section_stack);
  assembler->previous_section = -1;
  assembler->previous_subsection = 0;
  assembler->replay_current_line = false;
  assembler->stop_assembly = false;
}

void AssemblerMacroReset(Assembler* assembler) {
  UnwindNestedSources(assembler);
  MapTraverse(&assembler->macros, FreeMacroKV, NULL);
  MapClear(&assembler->macros);
  ClearCondStack(assembler);
  for (size_t i = 0; i < assembler->section_stack.length; i++) {
    free(assembler->section_stack.value.p[i]);
  }
  VectorClear(&assembler->section_stack);
  assembler->previous_section = -1;
  assembler->previous_subsection = 0;
  assembler->altmacro = false;
  assembler->macro_invocation_count = 0;
  assembler->macro_local_count = 0;
  assembler->replay_current_line = false;
  assembler->stop_assembly = false;
}

void AssemblerMacroShutdown(Assembler* assembler) {
  AssemblerMacroReset(assembler);
  MapDestruct(&assembler->macros);
  VectorDestruct(&assembler->expansion_frames);
  VectorDestruct(&assembler->cond_stack);
  VectorDestruct(&assembler->include_frames);
  VectorDestruct(&assembler->section_stack);
}
