//
//  linker_script.c
//  c_compiler
//
//  Parser for the commonly used GNU ld / LLVM lld linker-script subset:
//
//    MEMORY / PHDRS / SECTIONS / ENTRY / INCLUDE / REGION_ALIAS / EXTERN
//    Input globs: *(.text*), KEEP, SORT*, EXCLUDE_FILE, *(COMMON)
//    /DISCARD/, (NOLOAD), AT>, ALIGN/SUBALIGN, `. = expr`, PROVIDE/HIDDEN
//    ORIGIN()/LENGTH(), ONLY_IF_RO/RW, BYTE/SHORT/LONG/QUAD
//    Expressions: + - * / << >> & | ~, ALIGN(), CONSTANT(), SIZEOF_HEADERS,
//    MAX(), MIN(), ABSOLUTE(), K/M/G suffixes
//
//  Obscure features (VERSION, OVERLAY, INSERT, INPUT_SECTION_FLAGS, ...)
//  are skipped or rejected.
//

#include "linker_script.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elf.h"

typedef enum {
  kTokEof,
  kTokIdent,
  kTokNumber,
  kTokString,
  kTokLBrace,
  kTokRBrace,
  kTokLParen,
  kTokRParen,
  kTokColon,
  kTokSemi,
  kTokComma,
  kTokStar,
  kTokGt,
  kTokLt,
  kTokShl,
  kTokShr,
  kTokEq,
  kTokPlus,
  kTokMinus,
  kTokSlash,
  kTokAmp,
  kTokPipe,
  kTokTilde,
  kTokQuestion,
} TokenKind;

typedef struct {
  TokenKind kind;
  String text;
  uint64_t number;
  size_t start;
} Token;

typedef struct ScriptAst ScriptAst;

typedef struct {
  const char* filename;
  const char* src;
  size_t length;
  size_t pos;
  int lineno;
  int errors;
  int elf_machine_type;
  int include_depth;
  Token tok;
  ScriptAst* ast;
} ScriptParser;

typedef struct {
  String name;
  String attrs;
  String alias_of;
  uint64_t origin;
  uint64_t length;
} ScriptMemory;

typedef struct {
  String name;
  String type;
  uint64_t flags;
  bool has_flags;
} ScriptPhdr;

typedef struct {
  String output_name;
  Vector inputs;   // String*
  String memory;
  String phdr;
  uint64_t trailing_align;
  bool discard;
} ScriptSection;

typedef struct {
  String name;
  bool provide;
  bool image_end;
  bool has_absolute;
  uint64_t absolute;
  uint64_t align;
  Vector patterns;
} ScriptSymbol;

struct ScriptAst {
  Vector memories;  // ScriptMemory*
  Vector phdrs;     // ScriptPhdr*
  Vector sections;  // ScriptSection*
  Vector discards;  // String*
  Vector symbols;   // ScriptSymbol*
  String entry;
};

static void TokenInit(Token* tok) {
  tok->kind = kTokEof;
  StringInit(&tok->text, NULL);
  tok->number = 0;
  tok->start = 0;
}

static void TokenDestruct(Token* tok) {
  StringDestruct(&tok->text);
}

static void ScriptMemoryDestruct(ScriptMemory* m) {
  StringDestruct(&m->name);
  StringDestruct(&m->attrs);
  StringDestruct(&m->alias_of);
}

static void ScriptPhdrDestruct(ScriptPhdr* p) {
  StringDestruct(&p->name);
  StringDestruct(&p->type);
}

static void ScriptSectionDestruct(ScriptSection* s) {
  StringDestruct(&s->output_name);
  VectorDestructWithContents(&s->inputs, (VectorElementDestructor)StringDestruct,
                             /*free_element=*/true);
  StringDestruct(&s->memory);
  StringDestruct(&s->phdr);
}

static void ScriptSymbolDestruct(ScriptSymbol* s) {
  StringDestruct(&s->name);
  VectorDestructWithContents(&s->patterns,
                             (VectorElementDestructor)StringDestruct,
                             /*free_element=*/true);
}

static void ScriptAstInit(ScriptAst* ast) {
  VectorInit(&ast->memories);
  VectorInit(&ast->phdrs);
  VectorInit(&ast->sections);
  VectorInit(&ast->discards);
  VectorInit(&ast->symbols);
  StringInit(&ast->entry, NULL);
}

static void ScriptAstDestruct(ScriptAst* ast) {
  VectorDestructWithContents(&ast->memories,
                             (VectorElementDestructor)ScriptMemoryDestruct,
                             /*free_element=*/true);
  VectorDestructWithContents(&ast->phdrs,
                             (VectorElementDestructor)ScriptPhdrDestruct,
                             /*free_element=*/true);
  VectorDestructWithContents(&ast->sections,
                             (VectorElementDestructor)ScriptSectionDestruct,
                             /*free_element=*/true);
  VectorDestructWithContents(&ast->discards,
                             (VectorElementDestructor)StringDestruct,
                             /*free_element=*/true);
  VectorDestructWithContents(&ast->symbols,
                             (VectorElementDestructor)ScriptSymbolDestruct,
                             /*free_element=*/true);
  StringDestruct(&ast->entry);
}

static void ParserError(ScriptParser* p, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "%s:%d: ", p->filename ? p->filename : "<script>", p->lineno);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  p->errors++;
}

static int PeekChar(const ScriptParser* p) {
  if (p->pos >= p->length) {
    return EOF;
  }
  return (unsigned char)p->src[p->pos];
}

static int GetChar(ScriptParser* p) {
  if (p->pos >= p->length) {
    return EOF;
  }
  int ch = (unsigned char)p->src[p->pos++];
  if (ch == '\n') {
    p->lineno++;
  }
  return ch;
}

static void SkipLine(ScriptParser* p) {
  int ch;
  while ((ch = GetChar(p)) != EOF && ch != '\n') {
  }
}

static void SkipBlockComment(ScriptParser* p) {
  int ch;
  while ((ch = GetChar(p)) != EOF) {
    if (ch == '*' && PeekChar(p) == '/') {
      GetChar(p);
      return;
    }
  }
  ParserError(p, "Unterminated block comment");
}

static void SkipWhitespaceAndComments(ScriptParser* p) {
  for (;;) {
    int ch = PeekChar(p);
    if (ch == EOF) {
      return;
    }
    if (isspace(ch)) {
      GetChar(p);
      continue;
    }
    if (ch == '#') {
      SkipLine(p);
      continue;
    }
    if (ch == '/' && p->pos + 1 < p->length) {
      char next = p->src[p->pos + 1];
      if (next == '/') {
        SkipLine(p);
        continue;
      }
      if (next == '*') {
        GetChar(p);
        GetChar(p);
        SkipBlockComment(p);
        continue;
      }
    }
    return;
  }
}

static bool IsIdentStart(int ch) {
  return isalpha(ch) || ch == '_' || ch == '.' || ch == '$' || ch == '[';
}

static bool IsIdentCont(int ch) {
  return IsIdentStart(ch) || isdigit(ch) || ch == '-' || ch == '!' ||
         ch == ']';
}

static uint64_t ApplySizeSuffix(uint64_t value, int suffix) {
  switch (suffix) {
    case 'k':
    case 'K':
      return value * 1024ull;
    case 'm':
    case 'M':
      return value * 1024ull * 1024ull;
    case 'g':
    case 'G':
      return value * 1024ull * 1024ull * 1024ull;
    default:
      return value;
  }
}

static void NextToken(ScriptParser* p) {
  bool include_path =
      p->tok.kind == kTokIdent && strcmp(p->tok.text.value, "INCLUDE") == 0;
  TokenDestruct(&p->tok);
  TokenInit(&p->tok);
  SkipWhitespaceAndComments(p);
  p->tok.start = p->pos;
  int ch = PeekChar(p);
  if (ch == EOF) {
    p->tok.kind = kTokEof;
    return;
  }
  if (include_path && ch != '"' && ch != '\'') {
    p->tok.kind = kTokIdent;
    while (PeekChar(p) != EOF && !isspace(PeekChar(p)) &&
           PeekChar(p) != ';') {
      StringAppendChar(&p->tok.text, (char)GetChar(p));
    }
    return;
  }
  if (ch == '/' && p->length - p->pos >= strlen("/DISCARD/") &&
      strncmp(p->src + p->pos, "/DISCARD/", strlen("/DISCARD/")) == 0) {
    p->tok.kind = kTokIdent;
    StringAppend(&p->tok.text, "/DISCARD/");
    p->pos += strlen("/DISCARD/");
    return;
  }
  if (IsIdentStart(ch)) {
    p->tok.kind = kTokIdent;
    while (IsIdentCont(PeekChar(p))) {
      StringAppendChar(&p->tok.text, (char)GetChar(p));
    }
    return;
  }
  if (isdigit(ch)) {
    p->tok.kind = kTokNumber;
    int base = 10;
    if (ch == '0' && p->pos + 1 < p->length &&
        (p->src[p->pos + 1] == 'x' || p->src[p->pos + 1] == 'X')) {
      GetChar(p);
      GetChar(p);
      base = 16;
    }
    char* end = NULL;
    uint64_t value = strtoull(p->src + p->pos, &end, base);
    if (end == p->src + p->pos) {
      ParserError(p, "Invalid number");
      GetChar(p);
      p->tok.number = 0;
      return;
    }
    p->pos = (size_t)(end - p->src);
    int suffix = PeekChar(p);
    if (suffix == 'k' || suffix == 'K' || suffix == 'm' || suffix == 'M' ||
        suffix == 'g' || suffix == 'G') {
      value = ApplySizeSuffix(value, suffix);
      GetChar(p);
    }
    p->tok.number = value;
    return;
  }
  if (ch == '"' || ch == '\'') {
    int quote = GetChar(p);
    p->tok.kind = kTokString;
    for (;;) {
      int next = GetChar(p);
      if (next == EOF) {
        ParserError(p, "Unterminated string");
        break;
      }
      if (next == quote) {
        break;
      }
      if (next == '\\') {
        int esc = GetChar(p);
        if (esc == EOF) {
          ParserError(p, "Unterminated string escape");
          break;
        }
        StringAppendChar(&p->tok.text, (char)esc);
      } else {
        StringAppendChar(&p->tok.text, (char)next);
      }
    }
    return;
  }
  GetChar(p);
  switch (ch) {
    case '{':
      p->tok.kind = kTokLBrace;
      break;
    case '}':
      p->tok.kind = kTokRBrace;
      break;
    case '(':
      p->tok.kind = kTokLParen;
      break;
    case ')':
      p->tok.kind = kTokRParen;
      break;
    case ':':
      p->tok.kind = kTokColon;
      break;
    case ';':
      p->tok.kind = kTokSemi;
      break;
    case ',':
      p->tok.kind = kTokComma;
      break;
    case '*':
      p->tok.kind = kTokStar;
      break;
    case '>':
      if (PeekChar(p) == '>') {
        GetChar(p);
        p->tok.kind = kTokShr;
      } else {
        p->tok.kind = kTokGt;
      }
      break;
    case '<':
      if (PeekChar(p) == '<') {
        GetChar(p);
        p->tok.kind = kTokShl;
      } else {
        p->tok.kind = kTokLt;
      }
      break;
    case '=':
      p->tok.kind = kTokEq;
      break;
    case '+':
      p->tok.kind = kTokPlus;
      break;
    case '-':
      p->tok.kind = kTokMinus;
      break;
    case '/':
      p->tok.kind = kTokSlash;
      break;
    case '&':
      p->tok.kind = kTokAmp;
      break;
    case '|':
      p->tok.kind = kTokPipe;
      break;
    case '~':
      p->tok.kind = kTokTilde;
      break;
    case '?':
      p->tok.kind = kTokQuestion;
      break;
    default:
      ParserError(p, "Unexpected character '%c'", ch);
      p->tok.kind = kTokEof;
      break;
  }
}

static bool TokIsIdent(const Token* tok, const char* name) {
  return tok->kind == kTokIdent && strcmp(tok->text.value, name) == 0;
}

static bool Accept(ScriptParser* p, TokenKind kind) {
  if (p->tok.kind == kind) {
    NextToken(p);
    return true;
  }
  return false;
}

static bool AcceptIdent(ScriptParser* p, const char* name) {
  if (TokIsIdent(&p->tok, name)) {
    NextToken(p);
    return true;
  }
  return false;
}

static bool Expect(ScriptParser* p, TokenKind kind, const char* what) {
  if (Accept(p, kind)) {
    return true;
  }
  ParserError(p, "Expected %s", what);
  return false;
}

static void SkipBalanced(ScriptParser* p, TokenKind open, TokenKind close) {
  int depth = 1;
  while (p->tok.kind != kTokEof && depth > 0) {
    if (p->tok.kind == open) {
      depth++;
    } else if (p->tok.kind == close) {
      depth--;
    }
    NextToken(p);
  }
}

static uint64_t ParseExpression(ScriptParser* p);
static void ParseScript(ScriptParser* p, ScriptAst* ast);

static ScriptMemory* FindMemoryName(ScriptAst* ast, const char* name) {
  if (ast == NULL || name == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < ast->memories.length; i++) {
    ScriptMemory* mem = ast->memories.value.p[i];
    if (strcmp(mem->name.value, name) == 0) {
      return mem;
    }
  }
  return NULL;
}

static ScriptSymbol* FindScriptSymbol(ScriptAst* ast, const char* name) {
  if (ast == NULL || name == NULL) {
    return NULL;
  }
  for (size_t i = ast->symbols.length; i > 0; i--) {
    ScriptSymbol* symbol = ast->symbols.value.p[i - 1];
    if (strcmp(symbol->name.value, name) == 0) {
      return symbol;
    }
  }
  return NULL;
}

static uint64_t PageSizeForMachine(int machine) {
  (void)machine;
  return 0x1000;
}

static uint64_t SizeofHeadersForMachine(int machine) {
  bool elf64 = machine != ELF_MACHINE_TYPEW65C02 &&
               machine != ELF_MACHINE_TYPE_ARM &&
               machine != ELF_MACHINE_TYPE_X86 &&
               machine != ELF_MACHINE_TYPE_XTENSA;
  return elf64 ? 64ull + 4ull * 56ull : 52ull + 4ull * 32ull;
}

static uint64_t AlignUpExpr(uint64_t value, uint64_t align) {
  if (align <= 1) {
    return value;
  }
  return ((value + align - 1) / align) * align;
}

static uint64_t ParsePrimary(ScriptParser* p) {
  if (p->tok.kind == kTokNumber) {
    uint64_t value = p->tok.number;
    NextToken(p);
    return value;
  }
  if (Accept(p, kTokTilde)) {
    return ~ParsePrimary(p);
  }
  if (Accept(p, kTokMinus)) {
    return (uint64_t)(-(int64_t)ParsePrimary(p));
  }
  if (Accept(p, kTokLParen)) {
    uint64_t value = ParseExpression(p);
    Expect(p, kTokRParen, ")");
    return value;
  }
  if (p->tok.kind == kTokIdent) {
    if (TokIsIdent(&p->tok, ".") || TokIsIdent(&p->tok, "DOT")) {
      NextToken(p);
      return 0;
    }
    if (TokIsIdent(&p->tok, "SIZEOF_HEADERS")) {
      NextToken(p);
      return SizeofHeadersForMachine(p->elf_machine_type);
    }
    if (TokIsIdent(&p->tok, "ALIGN") || TokIsIdent(&p->tok, "BLOCK")) {
      NextToken(p);
      Expect(p, kTokLParen, "(");
      uint64_t first = ParseExpression(p);
      uint64_t value;
      if (Accept(p, kTokComma)) {
        value = AlignUpExpr(first, ParseExpression(p));
      } else {
        value = AlignUpExpr(0, first);
      }
      Expect(p, kTokRParen, ")");
      return value;
    }
    if (TokIsIdent(&p->tok, "CONSTANT")) {
      NextToken(p);
      Expect(p, kTokLParen, "(");
      uint64_t value = PageSizeForMachine(p->elf_machine_type);
      if (p->tok.kind == kTokIdent) {
        NextToken(p);
      }
      Expect(p, kTokRParen, ")");
      return value;
    }
    if (TokIsIdent(&p->tok, "MAX") || TokIsIdent(&p->tok, "MIN")) {
      bool is_max = TokIsIdent(&p->tok, "MAX");
      NextToken(p);
      Expect(p, kTokLParen, "(");
      uint64_t a = ParseExpression(p);
      Expect(p, kTokComma, ",");
      uint64_t b = ParseExpression(p);
      Expect(p, kTokRParen, ")");
      return is_max ? (a > b ? a : b) : (a < b ? a : b);
    }
    if (TokIsIdent(&p->tok, "ORIGIN") || TokIsIdent(&p->tok, "org") ||
        TokIsIdent(&p->tok, "LENGTH") || TokIsIdent(&p->tok, "len")) {
      bool want_length = TokIsIdent(&p->tok, "LENGTH") ||
                         TokIsIdent(&p->tok, "len");
      NextToken(p);
      Expect(p, kTokLParen, "(");
      uint64_t value = 0;
      if (p->tok.kind == kTokIdent || p->tok.kind == kTokString) {
        ScriptMemory* mem = FindMemoryName(p->ast, p->tok.text.value);
        if (mem != NULL) {
          value = want_length ? mem->length : mem->origin;
        } else {
          ParserError(p, "Unknown memory region '%s'", p->tok.text.value);
        }
        NextToken(p);
      } else {
        ParserError(p, "Expected memory region name");
      }
      Expect(p, kTokRParen, ")");
      return value;
    }
    if (TokIsIdent(&p->tok, "ABSOLUTE")) {
      NextToken(p);
      Expect(p, kTokLParen, "(");
      uint64_t value = ParseExpression(p);
      Expect(p, kTokRParen, ")");
      return value;
    }
    if (TokIsIdent(&p->tok, "DEFINED")) {
      NextToken(p);
      Expect(p, kTokLParen, "(");
      bool defined = false;
      if (p->tok.kind == kTokIdent) {
        defined = FindScriptSymbol(p->ast, p->tok.text.value) != NULL;
        NextToken(p);
      } else {
        ParserError(p, "DEFINED expects a symbol name");
      }
      Expect(p, kTokRParen, ")");
      return defined;
    }
    if (TokIsIdent(&p->tok, "ADDR") || TokIsIdent(&p->tok, "SIZEOF") ||
        TokIsIdent(&p->tok, "ALIGNOF") || TokIsIdent(&p->tok, "LOADADDR")) {
      NextToken(p);
      if (Accept(p, kTokLParen)) {
        ParseExpression(p);
        Expect(p, kTokRParen, ")");
      }
      return 0;
    }
    ScriptSymbol* symbol = FindScriptSymbol(p->ast, p->tok.text.value);
    NextToken(p);
    return symbol != NULL && symbol->has_absolute ? symbol->absolute : 0;
  }
  ParserError(p, "Expected expression");
  return 0;
}

static uint64_t ParseMul(ScriptParser* p) {
  uint64_t value = ParsePrimary(p);
  while (p->tok.kind == kTokStar || p->tok.kind == kTokSlash) {
    TokenKind op = p->tok.kind;
    NextToken(p);
    uint64_t rhs = ParsePrimary(p);
    if (op == kTokStar) {
      value *= rhs;
    } else if (rhs != 0) {
      value /= rhs;
    }
  }
  return value;
}

static uint64_t ParseAdd(ScriptParser* p) {
  uint64_t value = ParseMul(p);
  while (p->tok.kind == kTokPlus || p->tok.kind == kTokMinus) {
    TokenKind op = p->tok.kind;
    NextToken(p);
    uint64_t rhs = ParseMul(p);
    if (op == kTokPlus) {
      value += rhs;
    } else {
      value -= rhs;
    }
  }
  return value;
}

static uint64_t ParseShift(ScriptParser* p) {
  uint64_t value = ParseAdd(p);
  while (p->tok.kind == kTokShl || p->tok.kind == kTokShr) {
    TokenKind op = p->tok.kind;
    NextToken(p);
    uint64_t rhs = ParseAdd(p);
    if (op == kTokShl) {
      value <<= (rhs & 63);
    } else {
      value >>= (rhs & 63);
    }
  }
  return value;
}

static uint64_t ParseAnd(ScriptParser* p) {
  uint64_t value = ParseShift(p);
  while (Accept(p, kTokAmp)) {
    value &= ParseShift(p);
  }
  return value;
}

static uint64_t ParseOr(ScriptParser* p) {
  uint64_t value = ParseAnd(p);
  while (Accept(p, kTokPipe)) {
    value |= ParseAnd(p);
  }
  return value;
}

static uint64_t ParseExpression(ScriptParser* p) {
  uint64_t value = ParseOr(p);
  if (Accept(p, kTokQuestion)) {
    uint64_t then_v = ParseExpression(p);
    Expect(p, kTokColon, ":");
    uint64_t else_v = ParseExpression(p);
    return value ? then_v : else_v;
  }
  return value;
}

static void ParseMemoryCommand(ScriptParser* p, ScriptAst* ast) {
  if (!Expect(p, kTokLBrace, "{")) {
    return;
  }
  while (p->tok.kind != kTokRBrace && p->tok.kind != kTokEof) {
    if (p->tok.kind != kTokIdent) {
      ParserError(p, "Expected memory region name");
      NextToken(p);
      continue;
    }
    ScriptMemory* mem = calloc(1, sizeof(ScriptMemory));
    StringInit(&mem->name, p->tok.text.value);
    StringInit(&mem->attrs, NULL);
    StringInit(&mem->alias_of, NULL);
    NextToken(p);
    if (Accept(p, kTokLParen)) {
      if (p->tok.kind == kTokIdent || p->tok.kind == kTokString) {
        StringSet(&mem->attrs, p->tok.text.value);
        NextToken(p);
      }
      Expect(p, kTokRParen, ")");
    }
    Expect(p, kTokColon, ":");
    bool have_origin = false;
    bool have_length = false;
    while (p->tok.kind != kTokSemi && p->tok.kind != kTokRBrace &&
           p->tok.kind != kTokEof) {
      if (TokIsIdent(&p->tok, "ORIGIN") || TokIsIdent(&p->tok, "org") ||
          TokIsIdent(&p->tok, "o")) {
        NextToken(p);
        Expect(p, kTokEq, "=");
        mem->origin = ParseExpression(p);
        have_origin = true;
      } else if (TokIsIdent(&p->tok, "LENGTH") || TokIsIdent(&p->tok, "len") ||
                 TokIsIdent(&p->tok, "l")) {
        NextToken(p);
        Expect(p, kTokEq, "=");
        mem->length = ParseExpression(p);
        have_length = true;
      } else {
        // Next region name or closing brace; GNU ld does not require ';'.
        break;
      }
      Accept(p, kTokComma);
    }
    Accept(p, kTokSemi);
    if (!have_origin) {
      ParserError(p, "MEMORY region '%s' is missing ORIGIN", mem->name.value);
    }
    (void)have_length;
    VectorAppend(&ast->memories, mem);
  }
  Expect(p, kTokRBrace, "}");
}

static void ParsePhdrsCommand(ScriptParser* p, ScriptAst* ast) {
  if (!Expect(p, kTokLBrace, "{")) {
    return;
  }
  while (p->tok.kind != kTokRBrace && p->tok.kind != kTokEof) {
    if (p->tok.kind != kTokIdent) {
      ParserError(p, "Expected program header name");
      NextToken(p);
      continue;
    }
    ScriptPhdr* phdr = calloc(1, sizeof(ScriptPhdr));
    StringInit(&phdr->name, p->tok.text.value);
    StringInit(&phdr->type, NULL);
    NextToken(p);
    if (p->tok.kind == kTokIdent || p->tok.kind == kTokNumber) {
      if (p->tok.kind == kTokIdent) {
        StringSet(&phdr->type, p->tok.text.value);
      } else {
        StringPrintf(&phdr->type, "%llu", (unsigned long long)p->tok.number);
      }
      NextToken(p);
    }
    while (p->tok.kind != kTokSemi && p->tok.kind != kTokRBrace &&
           p->tok.kind != kTokEof) {
      if (AcceptIdent(p, "FLAGS")) {
        Expect(p, kTokLParen, "(");
        phdr->flags = ParseExpression(p);
        phdr->has_flags = true;
        Expect(p, kTokRParen, ")");
      } else if (AcceptIdent(p, "FILEHDR") || AcceptIdent(p, "PHDRS")) {
        continue;
      } else if (AcceptIdent(p, "AT")) {
        Expect(p, kTokLParen, "(");
        ParseExpression(p);
        Expect(p, kTokRParen, ")");
      } else {
        NextToken(p);
      }
    }
    Accept(p, kTokSemi);
    VectorAppend(&ast->phdrs, phdr);
  }
  Expect(p, kTokRBrace, "}");
}

static void AddInputPattern(Vector* inputs, const char* pattern) {
  if (pattern == NULL || pattern[0] == '\0') {
    return;
  }
  VectorAppend(inputs, NewString(pattern));
}

static void ParseSectionPattern(ScriptParser* p, String* out) {
  StringClear(out);
  if (p->tok.kind == kTokStar) {
    StringAppend(out, "*");
    NextToken(p);
  }
  if (p->tok.kind == kTokIdent) {
    StringAppend(out, p->tok.text.value);
    NextToken(p);
  }
  while (p->tok.kind == kTokStar || p->tok.kind == kTokQuestion) {
    StringAppendChar(out, p->tok.kind == kTokStar ? '*' : '?');
    size_t wildcard_end = p->pos;
    NextToken(p);
    if (p->tok.kind == kTokIdent && p->tok.start == wildcard_end) {
      StringAppend(out, p->tok.text.value);
      NextToken(p);
    }
  }
  if (out->length == 0) {
    ParserError(p, "Expected section name pattern");
    if (p->tok.kind != kTokRParen && p->tok.kind != kTokEof &&
        p->tok.kind != kTokRBrace) {
      NextToken(p);
    }
  }
}

static bool TokStartsSectionPattern(const Token* tok) {
  return tok->kind == kTokStar || tok->kind == kTokQuestion ||
         tok->kind == kTokIdent || tok->kind == kTokString;
}

static void ParsePatternList(ScriptParser* p, Vector* inputs) {
  while (p->tok.kind != kTokRParen && p->tok.kind != kTokEof) {
    if (TokIsIdent(&p->tok, "SORT") ||
        TokIsIdent(&p->tok, "SORT_BY_NAME") ||
        TokIsIdent(&p->tok, "SORT_BY_ALIGNMENT") ||
        TokIsIdent(&p->tok, "SORT_BY_INIT_PRIORITY") ||
        TokIsIdent(&p->tok, "SORT_NONE") ||
        TokIsIdent(&p->tok, "KEEP")) {
      NextToken(p);
      if (Expect(p, kTokLParen, "(")) {
        ParsePatternList(p, inputs);
        Expect(p, kTokRParen, ")");
      }
      continue;
    }
    if (TokIsIdent(&p->tok, "EXCLUDE_FILE") ||
        TokIsIdent(&p->tok, "INPUT_SECTION_FLAGS")) {
      NextToken(p);
      if (Accept(p, kTokLParen)) {
        SkipBalanced(p, kTokLParen, kTokRParen);
      }
      continue;
    }
    if (!TokStartsSectionPattern(&p->tok)) {
      ParserError(p, "Expected section name pattern");
      if (p->tok.kind != kTokRBrace) {
        NextToken(p);
      }
      break;
    }
    String pattern = {0};
    StringInit(&pattern, NULL);
    ParseSectionPattern(p, &pattern);
    AddInputPattern(inputs, pattern.value);
    StringDestruct(&pattern);
    Accept(p, kTokComma);
  }
}

static void ParseInputSectionList(ScriptParser* p, Vector* inputs);

static bool TokIsInputWrapper(const Token* tok) {
  return TokIsIdent(tok, "KEEP") || TokIsIdent(tok, "SORT") ||
         TokIsIdent(tok, "SORT_BY_NAME") ||
         TokIsIdent(tok, "SORT_BY_ALIGNMENT") ||
         TokIsIdent(tok, "SORT_BY_INIT_PRIORITY") ||
         TokIsIdent(tok, "SORT_NONE") || TokIsIdent(tok, "EXCLUDE_FILE") ||
         TokIsIdent(tok, "INPUT_SECTION_FLAGS");
}

static bool AcceptWrapper(ScriptParser* p, const char* name) {
  return AcceptIdent(p, name);
}

static void ParseInputSectionList(ScriptParser* p, Vector* inputs) {
  while (AcceptWrapper(p, "KEEP") || AcceptWrapper(p, "SORT") ||
         AcceptWrapper(p, "SORT_BY_NAME") ||
         AcceptWrapper(p, "SORT_BY_ALIGNMENT") ||
         AcceptWrapper(p, "SORT_BY_INIT_PRIORITY") ||
         AcceptWrapper(p, "SORT_NONE")) {
    if (!Expect(p, kTokLParen, "(")) {
      return;
    }
    ParseInputSectionList(p, inputs);
    Expect(p, kTokRParen, ")");
    return;
  }
  if (AcceptIdent(p, "EXCLUDE_FILE") ||
      AcceptIdent(p, "INPUT_SECTION_FLAGS")) {
    if (Accept(p, kTokLParen)) {
      SkipBalanced(p, kTokLParen, kTokRParen);
    }
    ParseInputSectionList(p, inputs);
    return;
  }
  if (p->tok.kind == kTokStar) {
    NextToken(p);
    if (p->tok.kind == kTokIdent) {
      // *filename(.sections)
      NextToken(p);
    }
    if (!Expect(p, kTokLParen, "(")) {
      return;
    }
    ParsePatternList(p, inputs);
    Expect(p, kTokRParen, ")");
    return;
  }
  if (p->tok.kind == kTokIdent) {
    String name = {0};
    StringInit(&name, p->tok.text.value);
    NextToken(p);
    if (Accept(p, kTokLParen)) {
      ParsePatternList(p, inputs);
      Expect(p, kTokRParen, ")");
      StringDestruct(&name);
      return;
    }
    AddInputPattern(inputs, name.value);
    StringDestruct(&name);
    return;
  }
  ParserError(p, "Expected input section description");
  if (p->tok.kind != kTokRBrace && p->tok.kind != kTokEof) {
    NextToken(p);
  }
}

static void ParseKeepOrInput(ScriptParser* p, ScriptSection* section) {
  ParseInputSectionList(p, &section->inputs);
}

static ScriptSymbol* NewScriptSymbol(const char* name, bool provide) {
  ScriptSymbol* sym = calloc(1, sizeof(ScriptSymbol));
  StringInit(&sym->name, name);
  VectorInit(&sym->patterns);
  sym->provide = provide;
  return sym;
}

static void CopyPatterns(Vector* dst, const Vector* src) {
  for (size_t i = 0; i < src->length; i++) {
    String* pattern = src->value.p[i];
    VectorAppend(dst, NewString(pattern->value));
  }
}

static void AddDotSymbol(ScriptParser* p, Vector* patterns, const char* name,
                         bool provide, uint64_t align, bool image_end) {
  ScriptSymbol* sym = NewScriptSymbol(name, provide);
  sym->align = align;
  sym->image_end = image_end;
  if (!image_end) {
    CopyPatterns(&sym->patterns, patterns);
  }
  VectorAppend(&p->ast->symbols, sym);
}

static bool ParseProvideOrAssign(ScriptParser* p, Vector* scope_patterns,
                                 bool image_end) {
  bool provide = false;
  if (AcceptIdent(p, "PROVIDE") || AcceptIdent(p, "PROVIDE_HIDDEN") ||
      AcceptIdent(p, "HIDDEN")) {
    provide = true;
    if (!Expect(p, kTokLParen, "(")) {
      return true;
    }
    if (p->tok.kind != kTokIdent) {
      ParserError(p, "Expected symbol name");
      SkipBalanced(p, kTokLParen, kTokRParen);
      return true;
    }
    String name = {0};
    StringInit(&name, p->tok.text.value);
    NextToken(p);
    Expect(p, kTokEq, "=");
    uint64_t align = 0;
    bool is_dot = TokIsIdent(&p->tok, ".") || TokIsIdent(&p->tok, "ALIGN") ||
                  TokIsIdent(&p->tok, "BLOCK");
    if (TokIsIdent(&p->tok, "ALIGN") || TokIsIdent(&p->tok, "BLOCK")) {
      NextToken(p);
      Expect(p, kTokLParen, "(");
      align = ParseExpression(p);
      if (Accept(p, kTokComma)) {
        ParseExpression(p);
      }
      Expect(p, kTokRParen, ")");
      is_dot = true;
    } else if (TokIsIdent(&p->tok, ".")) {
      NextToken(p);
    }
    uint64_t abs = 0;
    bool has_abs = false;
    if (!is_dot) {
      abs = ParseExpression(p);
      has_abs = true;
    }
    Expect(p, kTokRParen, ")");
    Accept(p, kTokSemi);
    if (has_abs) {
      ScriptSymbol* sym = NewScriptSymbol(name.value, provide);
      sym->has_absolute = true;
      sym->absolute = abs;
      VectorAppend(&p->ast->symbols, sym);
    } else {
      AddDotSymbol(p, scope_patterns, name.value, provide, align, image_end);
    }
    StringDestruct(&name);
    return true;
  }
  return false;
}

static void ParseOutputSection(ScriptParser* p, ScriptAst* ast,
                               const char* output_name) {
  ScriptSection* section = calloc(1, sizeof(ScriptSection));
  StringInit(&section->output_name, output_name);
  VectorInit(&section->inputs);
  StringInit(&section->memory, NULL);
  StringInit(&section->phdr, NULL);
  section->trailing_align = 0;
  section->discard = strcmp(output_name, "/DISCARD/") == 0;
  if (p->tok.kind == kTokNumber || p->tok.kind == kTokLParen ||
      (p->tok.kind == kTokIdent && !TokIsIdent(&p->tok, "ALIGN") &&
       !TokIsIdent(&p->tok, "SUBALIGN") &&
       !TokIsIdent(&p->tok, "ONLY_IF_RO") &&
       !TokIsIdent(&p->tok, "ONLY_IF_RW"))) {
    if (p->tok.kind != kTokColon) {
      ParseExpression(p);
    }
  }
  while (AcceptIdent(p, "ALIGN") || AcceptIdent(p, "SUBALIGN")) {
    Expect(p, kTokLParen, "(");
    ParseExpression(p);
    Expect(p, kTokRParen, ")");
  }
  AcceptIdent(p, "ONLY_IF_RO");
  AcceptIdent(p, "ONLY_IF_RW");
  if (Accept(p, kTokLParen)) {
    // Optional (NOLOAD) / (TYPE)
    while (p->tok.kind != kTokRParen && p->tok.kind != kTokEof) {
      NextToken(p);
    }
    Expect(p, kTokRParen, ")");
  }
  Expect(p, kTokColon, ":");
  if (AcceptIdent(p, "AT")) {
    Expect(p, kTokLParen, "(");
    ParseExpression(p);
    Expect(p, kTokRParen, ")");
  }
  if (!Expect(p, kTokLBrace, "{")) {
    ScriptSectionDestruct(section);
    free(section);
    return;
  }
  while (p->tok.kind != kTokRBrace && p->tok.kind != kTokEof) {
    if (p->tok.kind == kTokIdent && StringEqual(&p->tok.text, ".")) {
      NextToken(p);
      if (Accept(p, kTokEq)) {
        if (TokIsIdent(&p->tok, "ALIGN") || TokIsIdent(&p->tok, "BLOCK")) {
          NextToken(p);
          Expect(p, kTokLParen, "(");
          uint64_t align = ParseExpression(p);
          if (Accept(p, kTokComma)) {
            ParseExpression(p);
          }
          Expect(p, kTokRParen, ")");
          if (align > section->trailing_align) {
            section->trailing_align = align;
          }
        } else {
          ParseExpression(p);
        }
        Accept(p, kTokSemi);
        continue;
      }
    }
    if (AcceptIdent(p, "ASSERT") || AcceptIdent(p, "CONSTRUCTORS") ||
        AcceptIdent(p, "CREATE_OBJECT_SYMBOLS") || AcceptIdent(p, "FILL") ||
        AcceptIdent(p, "BYTE") || AcceptIdent(p, "SHORT") ||
        AcceptIdent(p, "LONG") || AcceptIdent(p, "QUAD") ||
        AcceptIdent(p, "SQUAD")) {
      if (Accept(p, kTokLParen)) {
        SkipBalanced(p, kTokLParen, kTokRParen);
      }
      Accept(p, kTokSemi);
      continue;
    }
    if (ParseProvideOrAssign(p, &section->inputs, false)) {
      continue;
    }
    if (p->tok.kind == kTokStar || TokIsInputWrapper(&p->tok)) {
      ParseKeepOrInput(p, section);
      Accept(p, kTokSemi);
      continue;
    }
    if (p->tok.kind == kTokIdent) {
      String name = {0};
      StringInit(&name, p->tok.text.value);
      NextToken(p);
      if (Accept(p, kTokEq)) {
        uint64_t align = 0;
        bool is_dot = TokIsIdent(&p->tok, ".") || TokIsIdent(&p->tok, "ALIGN") ||
                      TokIsIdent(&p->tok, "BLOCK");
        if (TokIsIdent(&p->tok, "ALIGN") || TokIsIdent(&p->tok, "BLOCK")) {
          NextToken(p);
          Expect(p, kTokLParen, "(");
          align = ParseExpression(p);
          if (Accept(p, kTokComma)) {
            ParseExpression(p);
          }
          Expect(p, kTokRParen, ")");
        } else if (TokIsIdent(&p->tok, ".")) {
          NextToken(p);
        }
        if (is_dot) {
          AddDotSymbol(p, &section->inputs, name.value, false, align, false);
        } else {
          ScriptSymbol* sym = NewScriptSymbol(name.value, false);
          sym->has_absolute = true;
          sym->absolute = ParseExpression(p);
          VectorAppend(&p->ast->symbols, sym);
        }
        Accept(p, kTokSemi);
        StringDestruct(&name);
        continue;
      }
      // file.o(.section) after we already consumed the filename.
      if (Accept(p, kTokLParen)) {
        ParsePatternList(p, &section->inputs);
        Expect(p, kTokRParen, ")");
        Accept(p, kTokSemi);
        StringDestruct(&name);
        continue;
      }
      ParserError(p, "Unexpected '%s' in output section", name.value);
      StringDestruct(&name);
      continue;
    }
    ParserError(p, "Unexpected token in output section");
    if (p->tok.kind != kTokRBrace && p->tok.kind != kTokEof) {
      NextToken(p);
    }
  }
  Expect(p, kTokRBrace, "}");
  while (p->tok.kind != kTokSemi && p->tok.kind != kTokRBrace &&
         p->tok.kind != kTokEof) {
    if (Accept(p, kTokGt)) {
      if (p->tok.kind == kTokIdent) {
        StringSet(&section->memory, p->tok.text.value);
        NextToken(p);
      } else {
        ParserError(p, "Expected memory region after '>'");
      }
    } else if (Accept(p, kTokColon)) {
      if (p->tok.kind == kTokIdent) {
        if (section->phdr.length == 0) {
          StringSet(&section->phdr, p->tok.text.value);
        }
        NextToken(p);
      } else {
        ParserError(p, "Expected program header after ':'");
      }
    } else if (Accept(p, kTokEq)) {
      ParseExpression(p);
    } else if (AcceptIdent(p, "AT")) {
      if (Accept(p, kTokGt)) {
        if (p->tok.kind == kTokIdent) {
          NextToken(p);
        }
      } else if (Accept(p, kTokLParen)) {
        ParseExpression(p);
        Expect(p, kTokRParen, ")");
      }
    } else {
      break;
    }
  }
  Accept(p, kTokSemi);
  if (section->discard) {
    CopyPatterns(&ast->discards, &section->inputs);
    ScriptSectionDestruct(section);
    free(section);
    return;
  }
  VectorAppend(&ast->sections, section);
}

static void ParseSectionsCommand(ScriptParser* p, ScriptAst* ast) {
  if (!Expect(p, kTokLBrace, "{")) {
    return;
  }
  while (p->tok.kind != kTokRBrace && p->tok.kind != kTokEof) {
    if (p->tok.kind == kTokIdent && StringEqual(&p->tok.text, ".")) {
      NextToken(p);
      if (Accept(p, kTokEq)) {
        ParseExpression(p);
        Accept(p, kTokSemi);
        continue;
      }
      ParserError(p, "Expected assignment after '.'");
      continue;
    }
    if (AcceptIdent(p, "ASSERT") || AcceptIdent(p, "OVERLAY")) {
      if (Accept(p, kTokLParen)) {
        SkipBalanced(p, kTokLParen, kTokRParen);
      } else if (Accept(p, kTokLBrace)) {
        SkipBalanced(p, kTokLBrace, kTokRBrace);
      }
      Accept(p, kTokSemi);
      continue;
    }
    if (ParseProvideOrAssign(p, &ast->discards, true)) {
      // Top-level PROVIDE(_end = .) is an image-end symbol.  The empty
      // discard vector is unused because image_end is set.
      continue;
    }
    if (p->tok.kind == kTokIdent) {
      String name = {0};
      StringInit(&name, p->tok.text.value);
      NextToken(p);
      if (Accept(p, kTokEq)) {
        if (TokIsIdent(&p->tok, ".") || TokIsIdent(&p->tok, "ALIGN") ||
            TokIsIdent(&p->tok, "BLOCK")) {
          uint64_t align = 0;
          if (TokIsIdent(&p->tok, "ALIGN") || TokIsIdent(&p->tok, "BLOCK")) {
            NextToken(p);
            Expect(p, kTokLParen, "(");
            align = ParseExpression(p);
            if (Accept(p, kTokComma)) {
              ParseExpression(p);
            }
            Expect(p, kTokRParen, ")");
          } else {
            NextToken(p);
          }
          AddDotSymbol(p, &ast->discards, name.value, false, align, true);
        } else {
          ScriptSymbol* sym = NewScriptSymbol(name.value, false);
          sym->has_absolute = true;
          sym->absolute = ParseExpression(p);
          VectorAppend(&ast->symbols, sym);
        }
        Accept(p, kTokSemi);
        StringDestruct(&name);
        continue;
      }
      ParseOutputSection(p, ast, name.value);
      StringDestruct(&name);
      continue;
    }
    ParserError(p, "Unexpected token in SECTIONS");
    NextToken(p);
  }
  Expect(p, kTokRBrace, "}");
}

static void ParseParenArgument(ScriptParser* p, String* out) {
  if (!Expect(p, kTokLParen, "(")) {
    return;
  }
  if (p->tok.kind == kTokIdent || p->tok.kind == kTokString) {
    if (out != NULL) {
      StringSet(out, p->tok.text.value);
    }
    NextToken(p);
  }
  Expect(p, kTokRParen, ")");
}

static char* ReadWholeFile(const char* filename, size_t* length) {
  FILE* fp = fopen(filename, "r");
  if (fp == NULL) {
    return NULL;
  }
  if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
    return NULL;
  }
  long size = ftell(fp);
  if (size < 0) {
    fclose(fp);
    return NULL;
  }
  rewind(fp);
  char* text = malloc((size_t)size + 1);
  size_t n = fread(text, 1, (size_t)size, fp);
  fclose(fp);
  text[n] = '\0';
  if (length != NULL) {
    *length = n;
  }
  return text;
}

static void ScriptDirName(const char* path, String* out) {
  if (path == NULL) {
    StringSet(out, ".");
    return;
  }
  const char* slash = strrchr(path, '/');
  if (slash == NULL) {
    StringSet(out, ".");
    return;
  }
  StringClear(out);
  StringAppendSegment(out, path, (size_t)(slash - path));
  if (out->length == 0) {
    StringSet(out, "/");
  }
}

static void IncludeScript(ScriptParser* parent, const char* name) {
  if (parent->include_depth >= 8) {
    ParserError(parent, "INCLUDE nested too deeply");
    return;
  }
  String path = {0};
  StringInit(&path, NULL);
  if (name[0] == '/') {
    StringSet(&path, name);
  } else {
    String dir = {0};
    StringInit(&dir, NULL);
    ScriptDirName(parent->filename, &dir);
    StringPrintf(&path, "%s/%s", dir.value, name);
    StringDestruct(&dir);
  }
  char* text = ReadWholeFile(path.value, NULL);
  if (text == NULL && name[0] != '/') {
    text = ReadWholeFile(name, NULL);
    if (text != NULL) {
      StringSet(&path, name);
    }
  }
  if (text == NULL) {
    ParserError(parent, "Cannot open INCLUDE file %s", name);
    StringDestruct(&path);
    return;
  }
  ScriptParser child = {0};
  child.filename = path.value;
  child.src = text;
  child.length = strlen(text);
  child.lineno = 1;
  child.elf_machine_type = parent->elf_machine_type;
  child.include_depth = parent->include_depth + 1;
  child.ast = parent->ast;
  TokenInit(&child.tok);
  ParseScript(&child, parent->ast);
  parent->errors += child.errors;
  TokenDestruct(&child.tok);
  free(text);
  StringDestruct(&path);
}

static void ParseScript(ScriptParser* p, ScriptAst* ast) {
  if (p->ast == NULL) {
    p->ast = ast;
  }
  NextToken(p);
  while (p->tok.kind != kTokEof) {
    if (AcceptIdent(p, "MEMORY")) {
      ParseMemoryCommand(p, ast);
    } else if (AcceptIdent(p, "PHDRS")) {
      ParsePhdrsCommand(p, ast);
    } else if (AcceptIdent(p, "SECTIONS")) {
      ParseSectionsCommand(p, ast);
    } else if (AcceptIdent(p, "ENTRY")) {
      ParseParenArgument(p, &ast->entry);
    } else if (AcceptIdent(p, "OUTPUT_ARCH") ||
               AcceptIdent(p, "OUTPUT_FORMAT") ||
               AcceptIdent(p, "SEARCH_DIR") ||
               AcceptIdent(p, "STARTUP") ||
               AcceptIdent(p, "TARGET") ||
               AcceptIdent(p, "OUTPUT")) {
      if (Accept(p, kTokLParen)) {
        SkipBalanced(p, kTokLParen, kTokRParen);
      }
    } else if (AcceptIdent(p, "INPUT") || AcceptIdent(p, "GROUP") ||
               AcceptIdent(p, "AS_NEEDED")) {
      if (Accept(p, kTokLParen)) {
        SkipBalanced(p, kTokLParen, kTokRParen);
      }
    } else if (AcceptIdent(p, "INCLUDE")) {
      if (p->tok.kind == kTokIdent || p->tok.kind == kTokString) {
        IncludeScript(p, p->tok.text.value);
        NextToken(p);
      } else {
        ParserError(p, "INCLUDE expects a file name");
        if (p->tok.kind != kTokEof) {
          NextToken(p);
        }
      }
    } else if (AcceptIdent(p, "REGION_ALIAS")) {
      Expect(p, kTokLParen, "(");
      String alias = {0};
      String target = {0};
      StringInit(&alias, NULL);
      StringInit(&target, NULL);
      if (p->tok.kind == kTokIdent || p->tok.kind == kTokString) {
        StringSet(&alias, p->tok.text.value);
        NextToken(p);
      } else {
        ParserError(p, "REGION_ALIAS expects an alias name");
      }
      Expect(p, kTokComma, ",");
      if (p->tok.kind == kTokIdent || p->tok.kind == kTokString) {
        StringSet(&target, p->tok.text.value);
        NextToken(p);
      } else {
        ParserError(p, "REGION_ALIAS expects a memory region");
      }
      Expect(p, kTokRParen, ")");
      Accept(p, kTokSemi);
      ScriptMemory* src = FindMemoryName(ast, target.value);
      if (src == NULL) {
        ParserError(p, "REGION_ALIAS target '%s' is unknown", target.value);
      } else {
        ScriptMemory* mem = calloc(1, sizeof(ScriptMemory));
        StringInit(&mem->name, alias.value);
        StringInit(&mem->attrs, src->attrs.value);
        StringInit(&mem->alias_of, src->name.value);
        mem->origin = src->origin;
        mem->length = src->length;
        VectorAppend(&ast->memories, mem);
      }
      StringDestruct(&alias);
      StringDestruct(&target);
    } else if (AcceptIdent(p, "EXTERN") || AcceptIdent(p, "NOCROSSREFS") ||
               AcceptIdent(p, "FORCE_COMMON_ALLOCATION") ||
               AcceptIdent(p, "INHIBIT_COMMON_ALLOCATION")) {
      if (Accept(p, kTokLParen)) {
        SkipBalanced(p, kTokLParen, kTokRParen);
      }
      Accept(p, kTokSemi);
    } else if (ParseProvideOrAssign(p, &ast->discards, true)) {
      continue;
    } else if (p->tok.kind == kTokIdent) {
      String name = {0};
      StringInit(&name, p->tok.text.value);
      NextToken(p);
      if (Accept(p, kTokEq)) {
        if (TokIsIdent(&p->tok, ".") || TokIsIdent(&p->tok, "ALIGN") ||
            TokIsIdent(&p->tok, "BLOCK")) {
          uint64_t align = 0;
          if (TokIsIdent(&p->tok, "ALIGN") || TokIsIdent(&p->tok, "BLOCK")) {
            NextToken(p);
            Expect(p, kTokLParen, "(");
            align = ParseExpression(p);
            if (Accept(p, kTokComma)) {
              ParseExpression(p);
            }
            Expect(p, kTokRParen, ")");
          } else {
            NextToken(p);
          }
          AddDotSymbol(p, &ast->discards, name.value, false, align, true);
        } else {
          ScriptSymbol* sym = NewScriptSymbol(name.value, false);
          sym->has_absolute = true;
          sym->absolute = ParseExpression(p);
          VectorAppend(&ast->symbols, sym);
        }
        Accept(p, kTokSemi);
      } else if (Accept(p, kTokLParen)) {
        SkipBalanced(p, kTokLParen, kTokRParen);
        Accept(p, kTokSemi);
      } else if (Accept(p, kTokLBrace)) {
        SkipBalanced(p, kTokLBrace, kTokRBrace);
      } else {
        ParserError(p, "Unknown linker-script command");
      }
      StringDestruct(&name);
    } else {
      ParserError(p, "Unexpected token in linker script");
      NextToken(p);
    }
  }
}

static ScriptMemory* FindMemory(ScriptAst* ast, const String* name) {
  if (name == NULL || name->length == 0) {
    return NULL;
  }
  return FindMemoryName(ast, name->value);
}

static ScriptPhdr* FindPhdr(ScriptAst* ast, const String* name) {
  if (name == NULL || name->length == 0) {
    return NULL;
  }
  for (size_t i = 0; i < ast->phdrs.length; i++) {
    ScriptPhdr* phdr = ast->phdrs.value.p[i];
    if (strcmp(phdr->name.value, name->value) == 0) {
      return phdr;
    }
  }
  return NULL;
}

static ConfigSegmentType SegmentTypeFromPhdr(const ScriptPhdr* phdr) {
  if (phdr == NULL) {
    return kConfigSegmentTypeUnknown;
  }
  if (strcmp(phdr->type.value, "PT_DYNAMIC") == 0 ||
      strcmp(phdr->type.value, "2") == 0) {
    return kConfigSegmentTypeDynamic;
  }
  if (strcmp(phdr->type.value, "PT_INTERP") == 0 ||
      strcmp(phdr->type.value, "3") == 0) {
    return kConfigSegmentTypeInterp;
  }
  if (strcmp(phdr->type.value, "PT_LOAD") == 0 ||
      strcmp(phdr->type.value, "1") == 0 ||
      phdr->type.length == 0) {
    if (phdr->has_flags) {
      return (phdr->flags & 1) ? kConfigSegmentTypeText
                               : kConfigSegmentTypeData;
    }
  }
  return kConfigSegmentTypeUnknown;
}

static ConfigSegmentType SegmentTypeFromAttrs(const String* attrs) {
  bool writable = false;
  bool executable = false;
  bool invert = false;
  if (attrs != NULL) {
    for (const char* p = attrs->value; *p != '\0'; p++) {
      if (*p == '!') {
        invert = !invert;
        continue;
      }
      bool on = !invert;
      if (*p == 'x') {
        executable = on;
      } else if (*p == 'w') {
        writable = on;
      }
    }
  }
  if (executable) {
    return kConfigSegmentTypeText;
  }
  if (writable) {
    return kConfigSegmentTypeData;
  }
  return kConfigSegmentTypeText;
}

static ConfigSegmentType SegmentTypeForMemory(ScriptAst* ast,
                                              const ScriptMemory* mem) {
  for (size_t i = 0; i < ast->sections.length; i++) {
    ScriptSection* section = ast->sections.value.p[i];
    if (section->memory.length > 0 &&
        strcmp(section->memory.value, mem->name.value) == 0) {
      ConfigSegmentType from_phdr =
          SegmentTypeFromPhdr(FindPhdr(ast, &section->phdr));
      if (from_phdr != kConfigSegmentTypeUnknown) {
        return from_phdr;
      }
    }
  }
  return SegmentTypeFromAttrs(&mem->attrs);
}

static ConfigRegion* FindRegionByName(ConfigSegment* segment,
                                      const char* name) {
  for (size_t i = 0; i < segment->regions.length; i++) {
    ConfigRegion* region = segment->regions.value.p[i];
    if (StringEqual(&region->name, name)) {
      return region;
    }
  }
  return NULL;
}

static bool RegionHasSection(const ConfigRegion* region, const char* name) {
  for (size_t i = 0; i < region->sections.length; i++) {
    String* existing = region->sections.value.p[i];
    if (StringEqual(existing, name)) {
      return true;
    }
  }
  return false;
}

static void ScriptAstToConfig(ScriptAst* ast, LinkerConfig* config) {
  if (ast->entry.length > 0) {
    StringSet(&config->entry_symbol, ast->entry.value);
  }
  for (size_t i = 0; i < ast->memories.length; i++) {
    ScriptMemory* mem = ast->memories.value.p[i];
    if (mem->alias_of.length > 0) {
      continue;
    }
    ConfigSegmentType type = SegmentTypeForMemory(ast, mem);
    ConfigSegment* segment = LinkerConfigAddSegment(config, type, 1);
    LinkerConfigAddRegion(segment, mem->name.value, mem->origin, mem->length,
                          false);
  }
  for (size_t i = 0; i < ast->sections.length; i++) {
    ScriptSection* section = ast->sections.value.p[i];
    ScriptMemory* mem = FindMemory(ast, &section->memory);
    ConfigSegmentType type = kConfigSegmentTypeText;
    const char* region_name = NULL;
    uint64_t origin = 0;
    uint64_t length = 0;
    if (mem != NULL) {
      ScriptMemory* storage =
          mem->alias_of.length > 0 ? FindMemoryName(ast, mem->alias_of.value)
                                   : mem;
      if (storage == NULL) {
        storage = mem;
      }
      type = SegmentTypeForMemory(ast, storage);
      region_name = storage->name.value;
      origin = storage->origin;
      length = storage->length;
    } else {
      ConfigSegmentType from_phdr =
          SegmentTypeFromPhdr(FindPhdr(ast, &section->phdr));
      if (from_phdr != kConfigSegmentTypeUnknown) {
        type = from_phdr;
      } else if (section->output_name.value[0] == '.' &&
                 (strcmp(section->output_name.value, ".data") == 0 ||
                  strcmp(section->output_name.value, ".bss") == 0 ||
                  strcmp(section->output_name.value, ".got") == 0 ||
                  strncmp(section->output_name.value, ".data.", 6) == 0)) {
        type = kConfigSegmentTypeData;
      }
      region_name = section->memory.length > 0 ? section->memory.value
                                               : section->output_name.value;
    }
    ConfigSegment* segment = LinkerConfigAddSegment(config, type, 1);
    ConfigRegion* region = FindRegionByName(segment, region_name);
    if (region == NULL) {
      region = LinkerConfigAddRegion(segment, region_name, origin, length,
                                     false);
    }
    for (size_t j = 0; j < section->inputs.length; j++) {
      String* input = section->inputs.value.p[j];
      if (!RegionHasSection(region, input->value)) {
        ConfigRegionAddSection(region, input->value);
      }
    }
    if (section->trailing_align > region->trailing_align) {
      region->trailing_align = section->trailing_align;
    }
  }
  CopyPatterns(&config->discard_patterns, &ast->discards);
  for (size_t i = 0; i < ast->symbols.length; i++) {
    ScriptSymbol* src = ast->symbols.value.p[i];
    ConfigScriptSymbol* dst = calloc(1, sizeof(ConfigScriptSymbol));
    StringInit(&dst->name, src->name.value);
    VectorInit(&dst->patterns);
    dst->provide = src->provide;
    dst->image_end = src->image_end;
    dst->has_absolute = src->has_absolute;
    dst->absolute = src->absolute;
    dst->align = src->align;
    CopyPatterns(&dst->patterns, &src->patterns);
    VectorAppend(&config->script_symbols, dst);
  }
  for (size_t i = 0; i < ast->phdrs.length; i++) {
    ScriptPhdr* phdr = ast->phdrs.value.p[i];
    ConfigSegmentType type = SegmentTypeFromPhdr(phdr);
    if (type == kConfigSegmentTypeDynamic || type == kConfigSegmentTypeInterp) {
      ConfigSegment* segment = LinkerConfigAddSegment(config, type, 1);
      if (segment->regions.length == 0) {
        LinkerConfigAddRegion(segment, phdr->name.value, 0, 0, false);
      }
    }
  }
}

static void ResetConfig(LinkerConfig* config) {
  LinkerConfigDestruct(config);
  LinkerConfigInitEmpty(config);
}

static bool ParseScriptText(const char* filename, const char* text,
                            int elf_machine_type, LinkerConfig* config) {
  ResetConfig(config);
  ScriptParser parser = {0};
  parser.filename = filename;
  parser.src = text != NULL ? text : "";
  parser.length = strlen(parser.src);
  parser.lineno = 1;
  parser.elf_machine_type = elf_machine_type;
  TokenInit(&parser.tok);
  ScriptAst ast;
  ScriptAstInit(&ast);
  parser.ast = &ast;
  ParseScript(&parser, &ast);
  if (parser.errors == 0) {
    ScriptAstToConfig(&ast, config);
    LinkerConfigEnsureSpecialSegments(config);
    LinkerConfigApplyTargetDefaults(config, elf_machine_type);
  } else {
    config->errors = parser.errors;
  }
  ScriptAstDestruct(&ast);
  TokenDestruct(&parser.tok);
  return parser.errors == 0;
}

bool LinkerScriptParseString(const char* filename, const char* text,
                             int elf_machine_type, LinkerConfig* config) {
  return ParseScriptText(filename, text, elf_machine_type, config);
}

bool LinkerScriptParseFile(const char* filename, int elf_machine_type,
                           LinkerConfig* config) {
  FILE* fp = fopen(filename, "r");
  if (fp == NULL) {
    ResetConfig(config);
    fprintf(stderr, "Cannot open linker script %s\n", filename);
    config->errors++;
    return false;
  }
  if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
    ResetConfig(config);
    fprintf(stderr, "Cannot read linker script %s\n", filename);
    config->errors++;
    return false;
  }
  long size = ftell(fp);
  if (size < 0) {
    fclose(fp);
    ResetConfig(config);
    fprintf(stderr, "Cannot read linker script %s\n", filename);
    config->errors++;
    return false;
  }
  rewind(fp);
  char* text = malloc((size_t)size + 1);
  size_t n = fread(text, 1, (size_t)size, fp);
  fclose(fp);
  text[n] = '\0';
  bool ok = ParseScriptText(filename, text, elf_machine_type, config);
  free(text);
  return ok;
}

// Built-in layouts.  These are ordinary GNU ld scripts and describe the
// same memory maps the old davecc dialect used for each architecture.

static const char kBuiltin6502Rom[] =
    "MEMORY\n"
    "{\n"
    "  text (rx) : ORIGIN = 0xc000, LENGTH = 0x3f00\n"
    "  boot (rx) : ORIGIN = 0xff00, LENGTH = 0xfa\n"
    "  hwvectors (r) : ORIGIN = 0xfffa, LENGTH = 6\n"
    "}\n"
    "SECTIONS\n"
    "{\n"
    "  .text : { *(.text* .rodata* .davecc_stacktrace) } > text\n"
    "  .boot : { *(.boot) } > boot\n"
    "  .hwvectors : { *(.hwvectors) } > hwvectors\n"
    "}\n";

static const char kBuiltin6502Program[] =
    "PHDRS\n"
    "{\n"
    "  interp PT_INTERP;\n"
    "}\n"
    "MEMORY\n"
    "{\n"
    "  text (rx) : ORIGIN = 0x800, LENGTH = 0\n"
    "  data (rw) : ORIGIN = 0, LENGTH = 0\n"
    "}\n"
    "SECTIONS\n"
    "{\n"
    "  .text : { *(.text* .rodata* .davecc_stacktrace) } > text\n"
    "  .data : { *(.data* .preinit_array* .init_array* .fini_array*) } > data\n"
    "  .bss : { *(.bss* COMMON) } > data\n"
    "}\n";

static const char kBuiltin6502Introm[] =
    "MEMORY\n"
    "{\n"
    "  text (rx) : ORIGIN = 0xc000, LENGTH = 0x3ffa\n"
    "  hwvectors (r) : ORIGIN = 0xfffa, LENGTH = 6\n"
    "}\n"
    "SECTIONS\n"
    "{\n"
    "  .text : { *(.text* .rodata*) } > text\n"
    "  .hwvectors : { *(.hwvectors) } > hwvectors\n"
    "}\n";

static const char kBuiltinRiscvProgram[] =
    "PHDRS\n"
    "{\n"
    "  text PT_LOAD FLAGS(5);\n"
    "  data PT_LOAD FLAGS(6);\n"
    "  dynamic PT_DYNAMIC;\n"
    "  interp PT_INTERP;\n"
    "}\n"
    "MEMORY\n"
    "{\n"
    "  text (rx) : ORIGIN = 0x400000000, LENGTH = 0\n"
    "  data (rw) : ORIGIN = 0x410000000, LENGTH = 0\n"
    "}\n"
    "SECTIONS\n"
    "{\n"
    "  .text : {\n"
    "    *(.text* .rodata* .davecc_stacktrace .eh_frame* .gcc_except_table)\n"
    "  } > text :text\n"
    "  .data : {\n"
    "    *(.got .got.plt .data* .preinit_array* .init_array* .fini_array*)\n"
    "  } > data :data\n"
    "  .bss : { *(.bss* COMMON) } > data\n"
    "  /DISCARD/ : { *(.comment .note .note.*) }\n"
    "}\n";

static const char kBuiltinRiscv32Program[] =
    "PHDRS\n"
    "{\n"
    "  text PT_LOAD FLAGS(5);\n"
    "  data PT_LOAD FLAGS(6);\n"
    "  dynamic PT_DYNAMIC;\n"
    "  interp PT_INTERP;\n"
    "}\n"
    "MEMORY\n"
    "{\n"
    "  text (rx) : ORIGIN = 0x40000000, LENGTH = 0\n"
    "  data (rw) : ORIGIN = 0x41000000, LENGTH = 0\n"
    "}\n"
    "SECTIONS\n"
    "{\n"
    "  .text : {\n"
    "    *(.text* .rodata* .davecc_stacktrace .eh_frame* .gcc_except_table)\n"
    "  } > text :text\n"
    "  .data : {\n"
    "    *(.got .got.plt .data* .preinit_array* .init_array* .fini_array*)\n"
    "  } > data :data\n"
    "  .bss : { *(.bss* COMMON) } > data\n"
    "  /DISCARD/ : { *(.comment .note .note.*) }\n"
    "}\n";

static const char kBuiltinEsp32Program[] =
    "PHDRS\n"
    "{\n"
    "  text PT_LOAD FLAGS(5);\n"
    "  data PT_LOAD FLAGS(6);\n"
    "}\n"
    "MEMORY\n"
    "{\n"
    "  iram (rx) : ORIGIN = 0x40080000, LENGTH = 0x20000\n"
    "  dram (rw) : ORIGIN = 0x3ffb0000, LENGTH = 0x50000\n"
    "}\n"
    "SECTIONS\n"
    "{\n"
    "  .text : {\n"
    "    *(.literal* .text* .rodata* .davecc_stacktrace .eh_frame* "
    ".gcc_except_table)\n"
    "  } > iram :text\n"
    "  .data : {\n"
    "    *(.data* .preinit_array* .init_array* .fini_array*)\n"
    "  } > dram :data\n"
    "  .bss : { *(.bss* COMMON) } > dram :data\n"
    "  .xtensa.info : { *(.xtensa.info) } > iram :text\n"
    "  /DISCARD/ : { *(.comment .note .note.*) }\n"
    "}\n";

static const char kBuiltinPcodeProgram[] =
    "PHDRS\n"
    "{\n"
    "  text PT_LOAD FLAGS(5);\n"
    "  data PT_LOAD FLAGS(6);\n"
    "  dynamic PT_DYNAMIC;\n"
    "  interp PT_INTERP;\n"
    "}\n"
    "MEMORY\n"
    "{\n"
    "  text (rx) : ORIGIN = 0x400000000, LENGTH = 0\n"
    "  data (rw) : ORIGIN = 0x410000000, LENGTH = 0\n"
    "}\n"
    "SECTIONS\n"
    "{\n"
    "  .text : {\n"
    "    *(.text* .rodata* .davecc_stacktrace .eh_frame* .gcc_except_table\n"
    "      .davecc_except_table)\n"
    "  } > text :text\n"
    "  .data : {\n"
    "    *(.got .got.plt .data* .preinit_array* .init_array* .fini_array*)\n"
    "  } > data :data\n"
    "  .bss : { *(.bss* COMMON) } > data\n"
    "  /DISCARD/ : { *(.comment .note .note.*) }\n"
    "}\n";

static const char kBuiltinAarch64Program[] =
    "PHDRS\n"
    "{\n"
    "  text PT_LOAD FLAGS(5);\n"
    "  data PT_LOAD FLAGS(6);\n"
    "  dynamic PT_DYNAMIC;\n"
    "  interp PT_INTERP;\n"
    "}\n"
    "MEMORY\n"
    "{\n"
    "  text (rx) : ORIGIN = 0x400000000, LENGTH = 0\n"
    "  data (rw) : ORIGIN = 0x410000000, LENGTH = 0\n"
    "}\n"
    "SECTIONS\n"
    "{\n"
    "  .text : {\n"
    "    *(.text* .rodata* .davecc_stacktrace .eh_frame* .gcc_except_table)\n"
    "  } > text :text\n"
    "  .data : {\n"
    "    *(.got .got.plt .data* .preinit_array* .init_array* .fini_array*)\n"
    "  } > data :data\n"
    "  .bss : { *(.bss* COMMON) } > data\n"
    "  /DISCARD/ : { *(.comment .note .note.*) }\n"
    "}\n";

static const char kBuiltinArmProgram[] =
    "PHDRS\n"
    "{\n"
    "  text PT_LOAD FLAGS(5);\n"
    "  data PT_LOAD FLAGS(6);\n"
    "  dynamic PT_DYNAMIC;\n"
    "  interp PT_INTERP;\n"
    "}\n"
    "MEMORY\n"
    "{\n"
    "  text (rx) : ORIGIN = 0x40000000, LENGTH = 0\n"
    "  data (rw) : ORIGIN = 0x41000000, LENGTH = 0\n"
    "}\n"
    "SECTIONS\n"
    "{\n"
    "  .text : {\n"
    "    *(.ARM.exidx* .text* .rodata* .davecc_stacktrace .gcc_except_table\n"
    "      .ARM.extab*)\n"
    "  } > text :text\n"
    "  .data : {\n"
    "    *(.got .got.plt .data* .preinit_array* .init_array* .fini_array*)\n"
    "  } > data :data\n"
    "  .bss : { *(.bss* COMMON) } > data\n"
    "  /DISCARD/ : { *(.comment .note .note.*) }\n"
    "}\n";

static const char kBuiltinX86_64Program[] =
    "PHDRS\n"
    "{\n"
    "  text PT_LOAD FLAGS(5);\n"
    "  data PT_LOAD FLAGS(6);\n"
    "  dynamic PT_DYNAMIC;\n"
    "  interp PT_INTERP;\n"
    "}\n"
    "MEMORY\n"
    "{\n"
    "  text (rx) : ORIGIN = 0x400000000, LENGTH = 0\n"
    "  data (rw) : ORIGIN = 0x410000000, LENGTH = 0\n"
    "}\n"
    "SECTIONS\n"
    "{\n"
    "  .text : {\n"
    "    *(.text* .rodata* .davecc_stacktrace .eh_frame* .gcc_except_table)\n"
    "  } > text :text\n"
    "  .data : {\n"
    "    *(.got .got.plt .data* .preinit_array* .init_array* .fini_array*)\n"
    "  } > data :data\n"
    "  .bss : { *(.bss* COMMON) } > data\n"
    "  /DISCARD/ : { *(.comment .note .note.*) }\n"
    "}\n";

static const char kBuiltinX86Program[] =
    "PHDRS\n"
    "{\n"
    "  text PT_LOAD FLAGS(5);\n"
    "  data PT_LOAD FLAGS(6);\n"
    "  dynamic PT_DYNAMIC;\n"
    "  interp PT_INTERP;\n"
    "}\n"
    "MEMORY\n"
    "{\n"
    "  text (rx) : ORIGIN = 0x08048000, LENGTH = 0\n"
    "  data (rw) : ORIGIN = 0x09000000, LENGTH = 0\n"
    "}\n"
    "SECTIONS\n"
    "{\n"
    "  .text : {\n"
    "    *(.text* .rodata* .davecc_stacktrace .eh_frame* .gcc_except_table)\n"
    "  } > text :text\n"
    "  .data : {\n"
    "    *(.got .got.plt .data* .preinit_array* .init_array* .fini_array*)\n"
    "  } > data :data\n"
    "  .bss : { *(.bss* COMMON) } > data\n"
    "  /DISCARD/ : { *(.comment .note .note.*) }\n"
    "}\n";

typedef struct {
  int machine;
  int elf_class;  // 0 matches either class.
  const char* type;
  const char* script;
} BuiltinScript;

static const BuiltinScript kBuiltinScripts[] = {
    {ELF_MACHINE_TYPEW65C02, 0, "rom", kBuiltin6502Rom},
    {ELF_MACHINE_TYPEW65C02, 0, "program", kBuiltin6502Program},
    {ELF_MACHINE_TYPEW65C02, 0, "introm", kBuiltin6502Introm},
    {ELF_MACHINE_TYPE_RISC_V, ELFCLASS32, "program", kBuiltinRiscv32Program},
    {ELF_MACHINE_TYPE_RISC_V, ELFCLASS64, "program", kBuiltinRiscvProgram},
    {ELF_MACHINE_TYPE_XTENSA, ELFCLASS32, "program", kBuiltinEsp32Program},
    {ELF_MACHINE_TYPE_PCODE, 0, "program", kBuiltinPcodeProgram},
    {ELF_MACHINE_TYPE_AARCH64, 0, "program", kBuiltinAarch64Program},
    {ELF_MACHINE_TYPE_ARM, 0, "program", kBuiltinArmProgram},
    {ELF_MACHINE_TYPE_X86, 0, "program", kBuiltinX86Program},
    {ELF_MACHINE_TYPE_X86_64, 0, "program", kBuiltinX86_64Program},
};

bool LinkerScriptLoadBuiltin(int elf_machine_type, bool is_64_bit,
                             const char* layout_type, LinkerConfig* config) {
  if (layout_type == NULL) {
    layout_type = "program";
  }
  int elf_class = is_64_bit ? ELFCLASS64 : ELFCLASS32;
  for (size_t i = 0; i < sizeof(kBuiltinScripts) / sizeof(kBuiltinScripts[0]);
       i++) {
    if (kBuiltinScripts[i].machine == elf_machine_type &&
        (kBuiltinScripts[i].elf_class == 0 ||
         kBuiltinScripts[i].elf_class == elf_class) &&
        strcmp(kBuiltinScripts[i].type, layout_type) == 0) {
      return LinkerScriptParseString("<builtin>", kBuiltinScripts[i].script,
                                     elf_machine_type, config);
    }
  }
  ResetConfig(config);
  fprintf(stderr, "Cannot find built-in linker script for machine %d layout %s\n",
          elf_machine_type, layout_type);
  config->errors++;
  return false;
}
