//
//  lex.c
//  c_compiler
//
//  Created by David Allison on 10/26/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

// This is the lexical analyzer.  It is responsible for reading the source
// code from the input file and converting it into "tokens" representing the
// components of the C language.  Tokens are reserved words (like int or while),
// identifiers, numbers operators and literals.
//
// An identifier is a variable name.
// A number is either an integer or floating point constant.
// An operator is a sequence of 1 or more characters that is meaningful
//   in the C language.  This includes things like +=, *, [, {, ...
// A literal is either a string (enclosed in double quotes) or a character
//   constant, enclosed in single quotes.
//
// If the input does not correspond to a valid token, the current token
// is set to TOK(bad) and this will result in a syntax error.
// At end of file, the current token will be set to TOK(eof).

// The Lexical Analyzer holds the current lexical state inside the
// Lex struct.  This is state consists of:
//
// a. The current token (the one we are looking at right now).
// b. The next character position in the input line.
// c. The spelling of the current token (for identifiers and strings).
// d. The value of an integer, character constant or floating point number.

// C is a free form language but still retains the concept of lines.  A line
// is terminated at a newline (ASCII 10) character on most operating systems
// (except Windows which uses a CRLR combination for historical reasons).
// The lexical syntax of C doesn't care much about lines except inside strings
// literals, character constants and comments.
//
// This lexical analyzer reads a line at a time into an internal string (there
// is no maximum line length).  Every time the lexical scan hits a newline
// character it will read another line, thus hiding the internal line
// processing from higher level functions.

#include "lex.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include <errno.h>
#include <limits.h>

#include "errors.h"
#include "vector.h"

// A reserved word, mapping a spelling to a token.
typedef struct {
  const char* spelling;
  Token token;
} ReservedWord;

typedef struct {
  const char* spelling;
  Token token;
  LanguageStandard min_standard;
} CXXReservedWord;

// All reserved words with associated token values, sorted in
// alphabetic order so we can do a binary search on them.
static ReservedWord reserved_words[] = {
  {"_Bool", TOK(bool)},
  {"_Complex", TOK(complex)},
  {"_Imaginary", TOK(imaginary)},
  {"__attribute__", TOK(attribute)},
  {"__thread", TOK(thread)},
  {"asm", TOK(asm)},
  {"auto", TOK(auto)},
  {"break", TOK(break)},
  {"case", TOK(case)},
  {"char", TOK(char)},
  {"const", TOK(const)},
  {"continue", TOK(continue)},
  {"default", TOK(default)},
  {"do", TOK(do)},
  {"double", TOK(double)},
  {"else", TOK(else)},
  {"enum", TOK(enum)},
  {"extern", TOK(extern)},
  {"float", TOK(float)},
  {"for", TOK(for)},
  {"goto", TOK(goto)},
  {"if", TOK(if)},
  {"inline", TOK(inline)},
  {"int", TOK(int)},
  {"long", TOK(long)},
  {"register", TOK(register)},
  {"restrict", TOK(restrict)},
  {"return", TOK(return)},
  {"short", TOK(short)},
  {"signed", TOK(signed)},
  {"sizeof", TOK(sizeof)},
  {"static", TOK(static)},
  {"struct", TOK(struct)},
  {"switch", TOK(switch)},
  {"typedef", TOK(typedef)},
  {"union", TOK(union)},
  {"unsigned", TOK(unsigned)},
  {"void", TOK(void)},
  {"volatile", TOK(volatile)},
  {"while", TOK(while)},
};

// C++ reserved words and alternative operator spellings.  This table is sorted
// alphabetically by spelling and is only used when a C++ -std= mode is active.
static CXXReservedWord cxx_reserved_words[] = {
  {"alignas", TOK(alignas), kLanguageStandardCXX11},
  {"alignof", TOK(alignof), kLanguageStandardCXX11},
  {"and", TOK(ampamp), kLanguageStandardCXX98},
  {"and_eq", TOK(ampeq), kLanguageStandardCXX98},
  {"asm", TOK(asm), kLanguageStandardCXX98},
  {"auto", TOK(auto), kLanguageStandardCXX98},
  {"bitand", TOK(amp), kLanguageStandardCXX98},
  {"bitor", TOK(bar), kLanguageStandardCXX98},
  {"bool", TOK(bool), kLanguageStandardCXX98},
  {"break", TOK(break), kLanguageStandardCXX98},
  {"case", TOK(case), kLanguageStandardCXX98},
  {"catch", TOK(catch), kLanguageStandardCXX98},
  {"char", TOK(char), kLanguageStandardCXX98},
  {"char16_t", TOK(char16_t), kLanguageStandardCXX11},
  {"char32_t", TOK(char32_t), kLanguageStandardCXX11},
  {"char8_t", TOK(char8_t), kLanguageStandardCXX20},
  {"class", TOK(class), kLanguageStandardCXX98},
  {"co_await", TOK(co_await), kLanguageStandardCXX20},
  {"co_return", TOK(co_return), kLanguageStandardCXX20},
  {"co_yield", TOK(co_yield), kLanguageStandardCXX20},
  {"compl", TOK(tilde), kLanguageStandardCXX98},
  {"concept", TOK(concept), kLanguageStandardCXX20},
  {"const", TOK(const), kLanguageStandardCXX98},
  {"const_cast", TOK(const_cast), kLanguageStandardCXX98},
  {"consteval", TOK(consteval), kLanguageStandardCXX20},
  {"constexpr", TOK(constexpr), kLanguageStandardCXX11},
  {"constinit", TOK(constinit), kLanguageStandardCXX20},
  {"continue", TOK(continue), kLanguageStandardCXX98},
  {"decltype", TOK(decltype), kLanguageStandardCXX11},
  {"default", TOK(default), kLanguageStandardCXX98},
  {"delete", TOK(delete), kLanguageStandardCXX98},
  {"do", TOK(do), kLanguageStandardCXX98},
  {"double", TOK(double), kLanguageStandardCXX98},
  {"dynamic_cast", TOK(dynamic_cast), kLanguageStandardCXX98},
  {"else", TOK(else), kLanguageStandardCXX98},
  {"enum", TOK(enum), kLanguageStandardCXX98},
  {"explicit", TOK(explicit), kLanguageStandardCXX98},
  {"export", TOK(export), kLanguageStandardCXX98},
  {"extern", TOK(extern), kLanguageStandardCXX98},
  {"false", TOK(false), kLanguageStandardCXX98},
  {"float", TOK(float), kLanguageStandardCXX98},
  {"for", TOK(for), kLanguageStandardCXX98},
  {"friend", TOK(friend), kLanguageStandardCXX98},
  {"goto", TOK(goto), kLanguageStandardCXX98},
  {"if", TOK(if), kLanguageStandardCXX98},
  {"import", TOK(import), kLanguageStandardCXX20},
  {"inline", TOK(inline), kLanguageStandardCXX98},
  {"int", TOK(int), kLanguageStandardCXX98},
  {"long", TOK(long), kLanguageStandardCXX98},
  {"module", TOK(module), kLanguageStandardCXX20},
  {"mutable", TOK(mutable), kLanguageStandardCXX98},
  {"namespace", TOK(namespace), kLanguageStandardCXX98},
  {"new", TOK(new), kLanguageStandardCXX98},
  {"noexcept", TOK(noexcept), kLanguageStandardCXX11},
  {"not", TOK(bang), kLanguageStandardCXX98},
  {"not_eq", TOK(bangeq), kLanguageStandardCXX98},
  {"nullptr", TOK(nullptr), kLanguageStandardCXX11},
  {"operator", TOK(operator), kLanguageStandardCXX98},
  {"or", TOK(barbar), kLanguageStandardCXX98},
  {"or_eq", TOK(bareq), kLanguageStandardCXX98},
  {"private", TOK(private), kLanguageStandardCXX98},
  {"protected", TOK(protected), kLanguageStandardCXX98},
  {"public", TOK(public), kLanguageStandardCXX98},
  {"register", TOK(register), kLanguageStandardCXX98},
  {"reinterpret_cast", TOK(reinterpret_cast), kLanguageStandardCXX98},
  {"requires", TOK(requires), kLanguageStandardCXX20},
  {"return", TOK(return), kLanguageStandardCXX98},
  {"short", TOK(short), kLanguageStandardCXX98},
  {"signed", TOK(signed), kLanguageStandardCXX98},
  {"sizeof", TOK(sizeof), kLanguageStandardCXX98},
  {"static", TOK(static), kLanguageStandardCXX98},
  {"static_assert", TOK(static_assert), kLanguageStandardCXX11},
  {"static_cast", TOK(static_cast), kLanguageStandardCXX98},
  {"struct", TOK(struct), kLanguageStandardCXX98},
  {"switch", TOK(switch), kLanguageStandardCXX98},
  {"template", TOK(template), kLanguageStandardCXX98},
  {"this", TOK(this), kLanguageStandardCXX98},
  {"thread_local", TOK(thread_local), kLanguageStandardCXX11},
  {"throw", TOK(throw), kLanguageStandardCXX98},
  {"true", TOK(true), kLanguageStandardCXX98},
  {"try", TOK(try), kLanguageStandardCXX98},
  {"typedef", TOK(typedef), kLanguageStandardCXX98},
  {"typeid", TOK(typeid), kLanguageStandardCXX98},
  {"typename", TOK(typename), kLanguageStandardCXX98},
  {"union", TOK(union), kLanguageStandardCXX98},
  {"unsigned", TOK(unsigned), kLanguageStandardCXX98},
  {"using", TOK(using), kLanguageStandardCXX98},
  {"virtual", TOK(virtual), kLanguageStandardCXX98},
  {"void", TOK(void), kLanguageStandardCXX98},
  {"volatile", TOK(volatile), kLanguageStandardCXX98},
  {"wchar_t", TOK(wchar_t), kLanguageStandardCXX98},
  {"while", TOK(while), kLanguageStandardCXX98},
  {"xor", TOK(caret), kLanguageStandardCXX98},
  {"xor_eq", TOK(careteq), kLanguageStandardCXX98},
};

// Number of reserved words in the array.
#define NUM_RESERVED_WORDS() (sizeof(reserved_words) / sizeof(ReservedWord))
#define NUM_CXX_RESERVED_WORDS() \
  (sizeof(cxx_reserved_words) / sizeof(CXXReservedWord))

static int CompareReservedWord(const void* a, const void* b) {
  const ReservedWord* word1 = a;
  const ReservedWord* word2 = b;
  return strcmp(word1->spelling, word2->spelling);
}

static int CompareCXXReservedWord(const void* a, const void* b) {
  const CXXReservedWord* word1 = a;
  const CXXReservedWord* word2 = b;
  return strcmp(word1->spelling, word2->spelling);
}

// Perform a binary search on the reserved_words array (sorted in alphabetic
// order of keyword) to find the given spelling.  If found, set *token
// to the token value and return true.
static bool IsReservedWord(const char* spelling, Token* token) {
  if (CompilerIsCXX()) {
    CXXReservedWord key;
    key.spelling = spelling;
    CXXReservedWord* value =
        bsearch(&key, cxx_reserved_words, NUM_CXX_RESERVED_WORDS(),
                sizeof(CXXReservedWord), CompareCXXReservedWord);
    if (value != NULL && CompilerCXXAtLeast(value->min_standard)) {
      *token = value->token;
      return true;
    }
    return false;
  }

  ReservedWord key;
  key.spelling = spelling;
  ReservedWord* value = bsearch(&key, reserved_words, NUM_RESERVED_WORDS(),
                                sizeof(ReservedWord), CompareReservedWord);
  if (value != NULL) {
    *token = value->token;
    return true;
  }
  return false;
}

// Get the next char while in a multi-line comment, reading another line
// if necessary.
static char GetCharInComment(Lex* lex) {
  if (lex->pos == lex->line.length) {
    LexReadLine(lex);
    return '\n';
  }
  return StringCharAt(&lex->line, lex->pos++);
}

// What is the current char?
static char CurrentChar(Lex* lex) { return StringCharAt(&lex->line, lex->pos); }

// What is the next char?
static char LookaheadChar(Lex* lex) { return lex->line.value[lex->pos + 1]; }

static bool CodepointInRange(uint32_t cp, uint32_t first, uint32_t last) {
  return cp >= first && cp <= last;
}

static bool CodepointAllowedInIdentifier(uint32_t cp) {
  // C99 Annex D.1 ranges for universal character names in identifiers.  Direct
  // UTF-8 source bytes are mapped to the same implementation-defined set.
  if (cp == 0x00a8 || cp == 0x00aa || cp == 0x00ad || cp == 0x00af ||
      CodepointInRange(cp, 0x00b2, 0x00b5) ||
      CodepointInRange(cp, 0x00b7, 0x00ba) ||
      CodepointInRange(cp, 0x00bc, 0x00be) ||
      CodepointInRange(cp, 0x00c0, 0x00d6) ||
      CodepointInRange(cp, 0x00d8, 0x00f6) ||
      CodepointInRange(cp, 0x00f8, 0x00ff) ||
      CodepointInRange(cp, 0x0100, 0x167f) ||
      CodepointInRange(cp, 0x1681, 0x180d) ||
      CodepointInRange(cp, 0x180f, 0x1fff) ||
      CodepointInRange(cp, 0x200b, 0x200d) ||
      CodepointInRange(cp, 0x202a, 0x202e) ||
      CodepointInRange(cp, 0x203f, 0x2040) || cp == 0x2054 ||
      CodepointInRange(cp, 0x2060, 0x206f) ||
      CodepointInRange(cp, 0x2070, 0x218f) ||
      CodepointInRange(cp, 0x2460, 0x24ff) ||
      CodepointInRange(cp, 0x2776, 0x2793) ||
      CodepointInRange(cp, 0x2c00, 0x2dff) ||
      CodepointInRange(cp, 0x2e80, 0x2fff) ||
      CodepointInRange(cp, 0x3004, 0x3007) ||
      CodepointInRange(cp, 0x3021, 0x302f) ||
      CodepointInRange(cp, 0x3031, 0x303f) ||
      CodepointInRange(cp, 0x3040, 0xd7ff) ||
      CodepointInRange(cp, 0xf900, 0xfd3d) ||
      CodepointInRange(cp, 0xfd40, 0xfdcf) ||
      CodepointInRange(cp, 0xfdf0, 0xfe44) ||
      CodepointInRange(cp, 0xfe47, 0xfffd)) {
    return true;
  }
  return cp >= 0x10000 && cp <= 0xefffd && (cp & 0xffff) <= 0xfffd;
}

static bool CodepointAllowedAtIdentifierStart(uint32_t cp) {
  return CodepointAllowedInIdentifier(cp) &&
         !CodepointInRange(cp, 0x0300, 0x036f) &&
         !CodepointInRange(cp, 0x1dc0, 0x1dff) &&
         !CodepointInRange(cp, 0x20d0, 0x20ff) &&
         !CodepointInRange(cp, 0xfe20, 0xfe2f);
}

static bool Utf8Continuation(unsigned char ch) { return (ch & 0xc0) == 0x80; }

static size_t DecodeUtf8(const char* text, size_t pos, size_t length,
                         uint32_t* codepoint) {
  if (pos >= length) {
    return 0;
  }
  unsigned char ch = (unsigned char)text[pos];
  if (ch < 0x80) {
    *codepoint = ch;
    return 1;
  }

  size_t needed;
  uint32_t cp;
  if (ch >= 0xc2 && ch <= 0xdf) {
    needed = 2;
    cp = ch & 0x1f;
  } else if (ch >= 0xe0 && ch <= 0xef) {
    needed = 3;
    cp = ch & 0x0f;
  } else if (ch >= 0xf0 && ch <= 0xf4) {
    needed = 4;
    cp = ch & 0x07;
  } else {
    return 0;
  }
  if (pos + needed > length) {
    return 0;
  }
  for (size_t i = 1; i < needed; i++) {
    unsigned char cont = (unsigned char)text[pos + i];
    if (!Utf8Continuation(cont)) {
      return 0;
    }
    cp = (cp << 6) | (cont & 0x3f);
  }

  if ((needed == 3 && cp < 0x800) || (needed == 4 && cp < 0x10000) ||
      (cp >= 0xd800 && cp <= 0xdfff) || cp > 0x10ffff) {
    return 0;
  }
  *codepoint = cp;
  return needed;
}

size_t LexIdentifierCharByteCount(const char* text, size_t pos, size_t length,
                                  bool start) {
  if (pos >= length) {
    return 0;
  }
  unsigned char ch = (unsigned char)text[pos];
  if (ch < 0x80) {
    if (start) {
      return isalpha(ch) || ch == '_' ? 1 : 0;
    }
    return isalnum(ch) || ch == '_' ? 1 : 0;
  }

  uint32_t cp;
  size_t bytes = DecodeUtf8(text, pos, length, &cp);
  if (bytes == 0) {
    return 0;
  }
  return (start ? CodepointAllowedAtIdentifierStart(cp)
                : CodepointAllowedInIdentifier(cp))
             ? bytes
             : 0;
}

// Perform escape processing on a char.  This handles
// backslashes inside a string literal or character constant.
static int EscapeChar(Lex* lex, int* size) {
  *size = 1;
  char ch = lex->line.value[lex->pos];
  if (ch == 'x' || ch == 'X') {
    lex->pos++;
    int n = 0;
    while (lex->pos < lex->line.length &&
           isxdigit((unsigned char)lex->line.value[lex->pos])) {
      ch = lex->line.value[lex->pos++];
      n <<= 4;
      if (isalpha((unsigned char)ch)) {
        n |= tolower(ch) - 'a' + 10;
      } else {
        n |= ch - '0';
      }
    }
    return n;
  } else if (ch == 'u' || ch == 'U') {
    // Universal character.
    int n = 0;
    int count = 4;
    lex->pos++;
    while (count > 0 && lex->pos < lex->line.length &&
           isxdigit((unsigned char)lex->line.value[lex->pos])) {
      ch = lex->line.value[lex->pos++];
      n <<= 4;
      if (isalpha((unsigned char)ch)) {
        n |= tolower(ch) - 'a' + 10;
      } else {
        n |= ch - '0';
      }
      count--;
    }
    if (count != 0) {
      LexError(lex, "A universal-character must have 4 hex digits");
    }
    *size = 4;
    return n;
  } else if (ch >= '0' && ch <= '7') {
    // Octal constant.
    int n = 0;
    while (lex->pos < lex->line.length && lex->line.value[lex->pos] >= '0' &&
           lex->line.value[lex->pos] <= '7') {
      ch = lex->line.value[lex->pos++];
      n = (n << 3) | ch - '0';
    }
    return n;
  } else {
    lex->pos++;
    switch (ch) {
      case 'n':
        return '\n';
      case 'r':
        return '\r';
      case 'b':
        return '\b';
      case '\\':
        return '\\';
      case '\'':
        return '\'';
      case '"':
        return '"';
      case '?':
        return '?';
      case 'a':
        return '\a';
      case 'f':
        return '\f';
      case 't':
        return '\t';
      case 'v':
        return '\v';
      default:
        LexError(lex, "Illegal escape sequence \\%c", ch);
    }
  }
  return ch;
}

// Collect an integer suffix.
// Allows U, UL, ULL, L, LL, LU, LLU
// If LLU or LU then it is reversed to ULL or UL.
static void CollectIntegerSuffix(Lex* lex) {
  StringClear(&lex->suffix);
  char ch = toupper(lex->line.value[lex->pos]);
  bool foundu = false;
  if (ch == 'U') {
    StringAppendChar(&lex->suffix, 'U');
    lex->pos++;
    foundu = true;
  }
  ch = toupper(lex->line.value[lex->pos]);
  if (ch == 'L') {
    StringAppendChar(&lex->suffix, 'L');
    lex->pos++;
  }
  ch = toupper(lex->line.value[lex->pos]);
  if (ch == 'L') {
    StringAppendChar(&lex->suffix, 'L');
    lex->pos++;
  }
  if (!foundu) {
    ch = toupper(lex->line.value[lex->pos]);
    if (ch == 'U') {
      char buf[16];
      buf[0] = 'U';
      strcpy(&buf[1], lex->suffix.value);
      StringSet(&lex->suffix, buf);
      lex->pos++;
    }
  }
}

// Collect a floating point suffix
// Allows F or L.
static void CollectFloatingSuffix(Lex* lex) {
  StringClear(&lex->suffix);
  char ch = toupper(lex->line.value[lex->pos]);
  if (ch == 'F') {
    StringAppendChar(&lex->suffix, 'F');
    lex->pos++;
  } else {
    ch = toupper(lex->line.value[lex->pos]);
    if (ch == 'L') {
      StringAppendChar(&lex->suffix, 'L');
      lex->pos++;
    }
  }
}

static void CollectUserDefinedLiteralSuffix(Lex* lex) {
  StringClear(&lex->ud_suffix);
  if (!CompilerCXXAtLeast(kLanguageStandardCXX11)) {
    return;
  }
  size_t bytes = LexIdentifierCharByteCount(lex->line.value, lex->pos,
                                            lex->line.length, true);
  if (bytes == 0) {
    return;
  }
  while (lex->pos < lex->line.length) {
    bytes = LexIdentifierCharByteCount(lex->line.value, lex->pos,
                                       lex->line.length, false);
    if (bytes == 0) {
      break;
    }
    StringAppendSegment(&lex->ud_suffix, &lex->line.value[lex->pos], bytes);
    lex->pos += bytes;
  }
}

static bool CharAt(Lex* lex, size_t offset, char ch) {
  return lex->pos + offset < lex->line.length &&
         lex->line.value[lex->pos + offset] == ch;
}

static void AppendRawLiteralChar(Lex* lex, LiteralEncoding encoding, char ch) {
  if (encoding == kLiteralEncodingWide) {
    for (int i = 0; i < compiler->wchar_size; i++) {
      StringAppendChar(&lex->spelling, ((unsigned char)ch >> i*8) & 0xff);
    }
    return;
  }
  StringAppendChar(&lex->spelling, ch);
}

static bool RawDelimiterChar(char ch) {
  return ch != ' ' && ch != '(' && ch != ')' && ch != '\\' &&
         ch != '\t' && ch != '\v' && ch != '\f' && ch != '\n';
}

static bool ReadRawStringContinuation(Lex* lex, LiteralEncoding encoding) {
  if (SourceEof(lex->source)) {
    return false;
  }
  AppendRawLiteralChar(lex, encoding, '\n');
  StringClear(&lex->line);
  SourceReadLine(lex->source, &lex->line);
  lex->pos = 0;
  return lex->line.length != 0 || !SourceEof(lex->source);
}

// Collect a C++ raw string literal.  lex->pos points at the R in R"...".
static void CollectRawStringLiteral(Lex* lex, LiteralEncoding encoding) {
  lex->literal_encoding = encoding;
  lex->literal_is_raw = true;
  StringClear(&lex->spelling);
  lex->pos += 2;  // Skip R".

  String delimiter = {0};
  StringInit(&delimiter, NULL);
  while (lex->pos < lex->line.length && lex->line.value[lex->pos] != '(') {
    char ch = lex->line.value[lex->pos++];
    if (!RawDelimiterChar(ch) || delimiter.length == 16) {
      LexError(lex, "Invalid raw string delimiter");
      StringDestruct(&delimiter);
      return;
    }
    StringAppendChar(&delimiter, ch);
  }
  if (lex->pos >= lex->line.length || lex->line.value[lex->pos] != '(') {
    LexError(lex, "Missing ( in raw string literal");
    StringDestruct(&delimiter);
    return;
  }
  lex->pos++;  // Skip (.

  bool closed = false;
  while (!SourceEof(lex->source) || lex->pos < lex->line.length) {
    if (lex->pos >= lex->line.length) {
      if (!ReadRawStringContinuation(lex, encoding)) {
        break;
      }
      continue;
    }
    char ch = lex->line.value[lex->pos];
    if (ch == ')' &&
        lex->pos + delimiter.length + 1 < lex->line.length &&
        strncmp(&lex->line.value[lex->pos + 1], delimiter.value,
                delimiter.length) == 0 &&
        lex->line.value[lex->pos + delimiter.length + 1] == '"') {
      lex->pos += delimiter.length + 2;
      closed = true;
      break;
    }
    AppendRawLiteralChar(lex, encoding, ch);
    lex->pos++;
  }
  if (!closed) {
    LexError(lex, "Unterminated raw string literal");
  }
  StringDestruct(&delimiter);
  CollectUserDefinedLiteralSuffix(lex);
}

// Collect a string literal into lex->spelling, omitting enclosing quotes.
// lex->pos is pointing at the open quote
static void CollectStringLiteral(Lex* lex) {
  lex->literal_encoding = kLiteralEncodingNone;
  lex->literal_is_raw = false;
  lex->pos++;
  StringClear(&lex->spelling);
  bool newline = lex->pos == lex->line.length;
  while (lex->pos < lex->line.length) {
    char ch = lex->line.value[lex->pos++];
    if (ch == '\n') {
      newline = true;
      break;
    }
    if (ch == '\\') {
      int size;
      int v = EscapeChar(lex, &size);
      for (int i = 0; i < size; i++) {
        StringAppendChar(&lex->spelling, (v >> i*8) & 0xff);
      }
    } else if (ch == '"') {
      break;
    } else {
      StringAppendChar(&lex->spelling, ch);
    }
  }
  if (newline) {
    LexError(lex, "Newline in string literal");
  }
  CollectUserDefinedLiteralSuffix(lex);
}

static void CollectWideStringLiteral(Lex* lex) {
  lex->literal_encoding = kLiteralEncodingWide;
  lex->literal_is_raw = false;
  lex->pos++;
  StringClear(&lex->spelling);
  bool newline = lex->pos == lex->line.length;
  while (lex->pos < lex->line.length) {
    char ch = lex->line.value[lex->pos++];
    if (ch == '\n') {
      newline = true;
      break;
    }
    if (ch == '\\') {
      int size;
      int v = EscapeChar(lex, &size);
      // Size is ignored.
      for (int i = 0; i < compiler->wchar_size; i++) {
        StringAppendChar(&lex->spelling, (v >> i*8) & 0xff);
      }
    } else if (ch == '"') {
      break;
    } else {
      // Decode a UTF-8 source sequence into a single Unicode code point and
      // store it as one wchar_t.  Each wide character holds a code point, not
      // an individual UTF-8 byte.
      unsigned int cp = (unsigned char)ch;
      int extra = 0;
      if ((cp & 0x80) != 0) {
        if ((cp & 0xe0) == 0xc0) {
          cp &= 0x1f;
          extra = 1;
        } else if ((cp & 0xf0) == 0xe0) {
          cp &= 0x0f;
          extra = 2;
        } else if ((cp & 0xf8) == 0xf0) {
          cp &= 0x07;
          extra = 3;
        }
        for (int k = 0; k < extra && lex->pos < lex->line.length; k++) {
          unsigned char cont = (unsigned char)lex->line.value[lex->pos];
          if ((cont & 0xc0) != 0x80) {
            break;   // Not a continuation byte; stop decoding.
          }
          cp = (cp << 6) | (cont & 0x3f);
          lex->pos++;
        }
      }
      for (int i = 0; i < compiler->wchar_size; i++) {
        StringAppendChar(&lex->spelling, (cp >> i*8) & 0xff);
      }
    }
  }
  if (newline) {
    LexError(lex, "Newline in string literal");
  }
  CollectUserDefinedLiteralSuffix(lex);
}
// Collect a character constant.  The current pos is the open single quote.
// Returns the binary value of the character constant.
static int CollectCharConst(Lex* lex) {
  lex->literal_encoding = kLiteralEncodingNone;
  lex->literal_is_raw = false;
  lex->pos++;
  int value = 0;
  int nchars = 0;
  bool newline = false;
  while (lex->pos < lex->line.length) {
    char ch = lex->line.value[lex->pos++];
    if (ch == '\n') {
      newline = true;
      break;
    }
    if (ch == '\\') {
      int size;
      int v = EscapeChar(lex, &size);
      for (int i = 0; i < size; i++) {
        value = (value << 8) | ((v >> i*8) & 0xff);
      }
      nchars += size;
    } else if (ch == '\'') {
      break;
    } else {
      value = (value << 8) | ch;
      nchars++;
    }
  }
  if (nchars > 4) {
    LexError(lex, "Max of 4 characters allowed in character constant");
  }
  if (nchars > 1) {
    LexWarning(lex, "multichar", "multi-character character constant");
  }
  if (nchars == 0 || newline) {
    LexError(lex, "Newline in character constant");
  }
  CollectUserDefinedLiteralSuffix(lex);
  return value;
}

static int CollectWideCharConst(Lex* lex) {
  lex->literal_encoding = kLiteralEncodingWide;
  lex->literal_is_raw = false;
  lex->pos++;
  int value = 0;
  int nchars = 0;
  bool newline = false;
  while  (lex->pos < lex->line.length) {
    char ch = lex->line.value[lex->pos++];
    if (ch == '\n') {
      newline = true;
      break;
    }
    if (ch == '\\') {
      int size;
      int v = EscapeChar(lex, &size);
      // Size is ignored.
      value = v;
      nchars ++;
    } else if (ch == '\'') {
      break;
    } else {
      value =  ch;
      nchars++;
    }
  }
  if (nchars > compiler->wchar_size) {
    LexError(lex, "Max of %d characters allowed in wide character constant", compiler->wchar_size);
  }
  if (nchars == 0 || newline) {
    LexError(lex, "Newline in character constant");
  }
  CollectUserDefinedLiteralSuffix(lex);
  return value;
}

// Is 'ch' a valid character for an identifier?  For regular C
// mode this is alphanumeric or '_'.
// For the assembler it also includes '.' and '@'.
// If 'start' then it cannot be numeric.
bool IsIdentifierChar(Lex* lex, char ch, bool start) {
  if (LexIdentifierCharByteCount(&ch, 0, 1, start) != 0) {
    return true;
  }
  // In assembler mode we allow . and @.
  if (lex->assembler_mode && (ch == '.' || ch == '@')) {
    return true;
  }
  return false;
}

// The current char is either the start of an identifier or the start
// of a wide string or char.  If it is an identifier, check for reserved
// word.
static void CollectIdentifierOrWide(Lex* lex) {
  if (CurrentChar(lex) == 'L' &&
      (LookaheadChar(lex) == '"' || LookaheadChar(lex) == '\'')) {
    if (LookaheadChar(lex) == '"') {
      lex->pos++;
      CollectWideStringLiteral(lex);
      lex->current_token = TOK(string_wide);
    } else {
      lex->pos++;
      lex->number = CollectWideCharConst(lex);
      lex->current_token = TOK(charconst_wide);
    }
    return;
  }

  if (CompilerIsCXX()) {
    LiteralEncoding encoding = kLiteralEncodingNone;
    size_t prefix_len = 0;
    bool raw = false;
    bool string_literal = false;
    bool char_literal = false;

    if (CharAt(lex, 0, 'R') && CharAt(lex, 1, '"')) {
      raw = true;
      string_literal = true;
    } else if (CharAt(lex, 0, 'L') && CharAt(lex, 1, 'R') &&
               CharAt(lex, 2, '"')) {
      encoding = kLiteralEncodingWide;
      prefix_len = 1;
      raw = true;
      string_literal = true;
    } else if (CharAt(lex, 0, 'u') && CharAt(lex, 1, '8')) {
      encoding = kLiteralEncodingUTF8;
      prefix_len = 2;
      if (CharAt(lex, 2, 'R') && CharAt(lex, 3, '"')) {
        raw = true;
        string_literal = true;
      } else if (CharAt(lex, 2, '"')) {
        string_literal = true;
      } else if (CharAt(lex, 2, '\'')) {
        char_literal = true;
      }
    } else if (CharAt(lex, 0, 'u')) {
      encoding = kLiteralEncodingUTF16;
      prefix_len = 1;
      if (CharAt(lex, 1, 'R') && CharAt(lex, 2, '"')) {
        raw = true;
        string_literal = true;
      } else if (CharAt(lex, 1, '"')) {
        string_literal = true;
      } else if (CharAt(lex, 1, '\'')) {
        char_literal = true;
      }
    } else if (CharAt(lex, 0, 'U')) {
      encoding = kLiteralEncodingUTF32;
      prefix_len = 1;
      if (CharAt(lex, 1, 'R') && CharAt(lex, 2, '"')) {
        raw = true;
        string_literal = true;
      } else if (CharAt(lex, 1, '"')) {
        string_literal = true;
      } else if (CharAt(lex, 1, '\'')) {
        char_literal = true;
      }
    }

    if (string_literal) {
      lex->pos += prefix_len;
      if (raw) {
        CollectRawStringLiteral(lex, encoding);
      } else {
        CollectStringLiteral(lex);
        lex->literal_encoding = encoding;
      }
      lex->current_token = encoding == kLiteralEncodingWide ? TOK(string_wide)
                                                            : TOK(string);
      return;
    }
    if (char_literal) {
      lex->pos += prefix_len;
      lex->number = CollectCharConst(lex);
      lex->literal_encoding = encoding;
      lex->current_token = TOK(charconst);
      return;
    }
  }

  // Identifier, collect into spelling.
  StringClear(&lex->spelling);
  while (lex->pos < lex->line.length) {
    char ch = lex->line.value[lex->pos];
    size_t bytes = LexIdentifierCharByteCount(lex->line.value, lex->pos,
                                              lex->line.length, false);
    if (bytes == 0 && lex->assembler_mode && (ch == '.' || ch == '@')) {
      bytes = 1;
    }
    if (bytes == 0) {
      break;
    }
    StringAppendSegment(&lex->spelling, &lex->line.value[lex->pos], bytes);
    lex->pos += bytes;
  }

  // In preprocessor and assembler modes we have no reserved words.
  if (lex->preprocessor_mode || lex->assembler_mode) {
    lex->current_token = TOK(identifier);
    return;
  }

  // Identifier or reserved word.
  if (!IsReservedWord(lex->spelling.value, &lex->current_token)) {
    lex->current_token = TOK(identifier);
  }
}

// Collect an operator token.  This is done using a simple switch statement in
// order to make it clear what is happening and how it works.  There are other
// more complex alternatives to this but I think this is clearest.
// Sets the current_token to the token found (if any) and moves the
// position to after the token characters.
static void CollectOperator(Lex* lex) {
  char ch = lex->line.value[lex->pos];
  switch (ch) {
    case '+':
      ch = lex->line.value[++lex->pos];
      if (ch == '=') {
        lex->current_token = TOK(pluseq);
        lex->pos++;
      } else if (ch == '+') {
        lex->current_token = TOK(plusplus);
        lex->pos++;
      } else {
        lex->current_token = TOK(plus);
      }
      break;
    case '-':
      ch = lex->line.value[++lex->pos];
      if (ch == '=') {
        lex->current_token = TOK(minuseq);
        lex->pos++;
      } else if (ch == '-') {
        lex->current_token = TOK(minusminus);
        lex->pos++;
      } else if (ch == '>') {
        lex->pos++;
        if (CompilerIsCXX() && lex->line.value[lex->pos] == '*') {
          lex->current_token = TOK(arrowstar);
          lex->pos++;
        } else {
          lex->current_token = TOK(arrow);
        }
      } else {
        lex->current_token = TOK(minus);
      }
      break;

    case '*':
      ch = lex->line.value[++lex->pos];
      if (ch == '=') {
        lex->current_token = TOK(stareq);
        lex->pos++;
      } else {
        lex->current_token = TOK(star);
      }
      break;

    case '/':
      ch = lex->line.value[++lex->pos];
      if (ch == '=') {
        lex->current_token = TOK(slasheq);
        lex->pos++;
      } else {
        lex->current_token = TOK(slash);
      }
      break;

    case '(':
      lex->current_token = TOK(lparen);
      lex->pos++;
      break;
    case ')':
      lex->current_token = TOK(rparen);
      lex->pos++;
      break;
    case '[':
      lex->current_token = TOK(lsquare);
      lex->pos++;
      break;
    case ']':
      lex->current_token = TOK(rsquare);
      lex->pos++;
      break;
    case '{':
      lex->current_token = TOK(lbrace);
      lex->pos++;
      break;
    case '}':
      lex->current_token = TOK(rbrace);
      lex->pos++;
      break;
    case '=':
      ch = lex->line.value[++lex->pos];
      if (ch == '=') {
        lex->current_token = TOK(equalequal);
        lex->pos++;
      } else {
        lex->current_token = TOK(equal);
      }
      break;
    case '>':
      ch = lex->line.value[++lex->pos];
      if (ch == '=') {
        lex->current_token = TOK(greatereq);
        lex->pos++;
      } else if (ch == '>') {
        ch = lex->line.value[++lex->pos];
        if (ch == '=') {
          lex->current_token = TOK(greatergreatereq);
          lex->pos++;
        } else {
          lex->current_token = TOK(greatergreater);
        }
      } else {
        lex->current_token = TOK(greater);
      }
      break;
    case '<':
      ch = lex->line.value[++lex->pos];
      if (CompilerIsCXX() && ch == '=' &&
          lex->line.value[lex->pos + 1] == '>') {
        lex->current_token = TOK(spaceship);
        lex->pos += 2;
      } else if (ch == '=') {
        lex->current_token = TOK(lesseq);
        lex->pos++;
      } else if (ch == '<') {
        ch = lex->line.value[++lex->pos];
        if (ch == '=') {
          lex->current_token = TOK(lesslesseq);
          lex->pos++;
        } else {
          lex->current_token = TOK(lessless);
        }
      } else {
        lex->current_token = TOK(less);
      }
      break;
    case '%':
      ch = lex->line.value[++lex->pos];
      if (ch == '=') {
        lex->current_token = TOK(percenteq);
        lex->pos++;
      } else {
        lex->current_token = TOK(percent);
      }
      break;
    case '&':
      ch = lex->line.value[++lex->pos];
      if (ch == '=') {
        lex->current_token = TOK(ampeq);
        lex->pos++;
      } else if (ch == '&') {
        lex->current_token = TOK(ampamp);
        lex->pos++;
      } else {
        lex->current_token = TOK(amp);
      }
      break;
    case '!':
      ch = lex->line.value[++lex->pos];
      if (ch == '=') {
        lex->current_token = TOK(bangeq);
        lex->pos++;
      } else {
        lex->current_token = TOK(bang);
      }
      break;
    case '^':
      ch = lex->line.value[++lex->pos];
      if (ch == '=') {
        lex->current_token = TOK(careteq);
        lex->pos++;
      } else {
        lex->current_token = TOK(caret);
      }
      break;
    case '|':
      ch = lex->line.value[++lex->pos];
      if (ch == '=') {
        lex->current_token = TOK(bareq);
        lex->pos++;
      } else if (ch == '|') {
        lex->current_token = TOK(barbar);
        lex->pos++;
      } else {
        lex->current_token = TOK(bar);
      }
      break;
    case ',':
      lex->current_token = TOK(comma);
      lex->pos++;
      break;

    case '.':
      ch = lex->line.value[++lex->pos];
      if (ch == '.' && lex->line.value[lex->pos + 1] == '.') {
        // Check for "..." (ellipsis)
        lex->current_token = TOK(ellipsis);
        lex->pos += 2;
      } else if (CompilerIsCXX() && ch == '*') {
        lex->current_token = TOK(dotstar);
        lex->pos++;
      } else {
        lex->current_token = TOK(dot);
      }
      break;

    case '?':
      lex->current_token = TOK(question);
      lex->pos++;
      break;
    case ':':
      lex->pos++;
      if (CompilerIsCXX() && lex->line.value[lex->pos] == ':') {
        lex->current_token = TOK(coloncolon);
        lex->pos++;
      } else {
        lex->current_token = TOK(colon);
      }
      break;

    case ';':
      lex->current_token = TOK(semicolon);
      lex->pos++;
      break;

    case '~':
      lex->current_token = TOK(tilde);
      lex->pos++;
      break;

    // # is a token in assembler mode.
    case '#':
      if (lex->assembler_mode) {
        lex->current_token = TOK(hash);
        lex->pos++;
      }
      break;
    default:
      // No a valid operator.  Leave current_token as is.
      break;
  }
}

static void InitCommon(Lex* lex, Preprocessor* preprocessor) {
  lex->current_token = TOK(bad);
  StringInit(&lex->line, NULL);
  StringInit(&lex->spelling, NULL);
  StringInit(&lex->suffix, NULL);
  StringInit(&lex->ud_suffix, NULL);
  lex->number = 0;
  lex->fnumber = 0;
  lex->literal_encoding = kLiteralEncodingNone;
  lex->literal_is_raw = false;
  lex->pos = 0;
  lex->preprocessor = preprocessor;
  lex->preprocessor_mode = false;
  lex->assembler_mode = false;
  lex->in_comment = false;
  preprocessor->lex = lex;
}

// Initialize a lexical analyzer from a file.  The special filename "-" means
// read the translation unit from standard input.
bool LexInitFromFile(Lex* lex, const char* filename,
                     Preprocessor* preprocessor) {
  if (strcmp(filename, "-") == 0) {
    // stdin is not seekable, but the lexer relies on save/restore checkpoints
    // (which seek the source), so slurp all of standard input into a string
    // source, which is seekable by index.
    String* code = NewString(NULL);
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), stdin)) > 0) {
      StringAppendSegment(code, buf, n);
    }
    lex->source = NewSourceFromString("<stdin>", code);
    InitCommon(lex, preprocessor);
    return true;
  }
  FILE* in = fopen(filename, "r");
  if (in == NULL) {
    fprintf(stderr, "No such file %s\n", filename);
    return false;
  }
  lex->source = NewSourceFromFile(filename, in);
  InitCommon(lex, preprocessor);
  return true;
}

bool LexInitFromString(Lex* lex, const char* filename, String* code,
                       Preprocessor* preprocessor) {
  lex->source = NewSourceFromString(filename, code);
  InitCommon(lex, preprocessor);
  lex->source->path_index = preprocessor->lex->source->path_index;
  return true;
}

void LexCheckpointSave(Lex* lex, LexCheckpoint* checkpoint) {
  checkpoint->source = lex->source;
  checkpoint->source_device = lex->source->device;
  if (lex->source->device == kSourceFromFile) {
    fgetpos(lex->source->from.file, &checkpoint->file_pos);
    checkpoint->string_index = 0;
  } else {
    checkpoint->string_index = lex->source->from.string.index;
  }
  checkpoint->lineno = lex->source->lineno;
  checkpoint->file_index = lex->source->file_index;
  checkpoint->path_index = lex->source->path_index;

  StringInitFromSegment(&checkpoint->line, lex->line.value, lex->line.length);
  checkpoint->pos = lex->pos;
  checkpoint->current_token_location = lex->current_token_location;
  checkpoint->current_token = lex->current_token;
  StringInitFromSegment(&checkpoint->spelling, lex->spelling.value,
                        lex->spelling.length);
  checkpoint->number = lex->number;
  checkpoint->fnumber = lex->fnumber;
  StringInitFromSegment(&checkpoint->suffix, lex->suffix.value,
                        lex->suffix.length);
  StringInitFromSegment(&checkpoint->ud_suffix, lex->ud_suffix.value,
                        lex->ud_suffix.length);
  checkpoint->literal_encoding = lex->literal_encoding;
  checkpoint->literal_is_raw = lex->literal_is_raw;
  checkpoint->preprocessor_mode = lex->preprocessor_mode;
  checkpoint->in_comment = lex->in_comment;
  checkpoint->assembler_mode = lex->assembler_mode;
}

void LexCheckpointRestore(Lex* lex, LexCheckpoint* checkpoint) {
  lex->source = checkpoint->source;
  if (checkpoint->source_device == kSourceFromFile) {
    fsetpos(lex->source->from.file, &checkpoint->file_pos);
    clearerr(lex->source->from.file);
  } else {
    lex->source->from.string.index = checkpoint->string_index;
  }
  lex->source->lineno = checkpoint->lineno;
  lex->source->file_index = checkpoint->file_index;
  lex->source->path_index = checkpoint->path_index;

  StringSetString(&lex->line, &checkpoint->line);
  lex->pos = checkpoint->pos;
  lex->current_token_location = checkpoint->current_token_location;
  lex->current_token = checkpoint->current_token;
  StringSetString(&lex->spelling, &checkpoint->spelling);
  lex->number = checkpoint->number;
  lex->fnumber = checkpoint->fnumber;
  StringSetString(&lex->suffix, &checkpoint->suffix);
  StringSetString(&lex->ud_suffix, &checkpoint->ud_suffix);
  lex->literal_encoding = checkpoint->literal_encoding;
  lex->literal_is_raw = checkpoint->literal_is_raw;
  lex->preprocessor_mode = checkpoint->preprocessor_mode;
  lex->in_comment = checkpoint->in_comment;
  lex->assembler_mode = checkpoint->assembler_mode;
}

void LexCheckpointDestruct(LexCheckpoint* checkpoint) {
  StringDestruct(&checkpoint->line);
  StringDestruct(&checkpoint->spelling);
  StringDestruct(&checkpoint->suffix);
  StringDestruct(&checkpoint->ud_suffix);
}

// Destruct a lexical analyzer.
void LexDestruct(Lex* lex) {
  if (lex->source != NULL) {
    SourceDestruct(lex->source);
    free(lex->source);
  }

  StringDestruct(&lex->line);
  StringDestruct(&lex->spelling);
  StringDestruct(&lex->suffix);
  StringDestruct(&lex->ud_suffix);
}

// Collects a hexadecimal number prefixed by a $.
static void CollectHex(Lex* lex) {
  int64_t number = 0;
  // Collect hex number and convert to binary.
  while (lex->pos < lex->line.length &&
         isxdigit((unsigned char)lex->line.value[lex->pos])) {
    char ch = lex->line.value[lex->pos++];
    number <<= 4;
    if (isalpha((unsigned char)ch)) {
      number |= tolower(ch) - 'a' + 10;
    } else {
      number |= ch - '0';
    }
  }
  lex->number = number;
  CollectIntegerSuffix(lex);
  lex->current_token = TOK(number);
}

static bool IsDigitSeparator(Lex* lex, bool ishex, bool isoctal,
                             bool isbinary) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX14) ||
      lex->line.value[lex->pos] != '\'' || lex->pos + 1 >= lex->line.length) {
    return false;
  }
  char next = lex->line.value[lex->pos + 1];
  if (isbinary) {
    return next == '0' || next == '1';
  }
  if (ishex) {
    return isxdigit((unsigned char)next);
  }
  if (isoctal) {
    return next >= '0' && next <= '7';
  }
  return isdigit((unsigned char)next);
}

static void CollectNumber(Lex* lex, char ch) {
  if (lex->assembler_mode && ch == '$') {
    lex->pos++;     // Skip $.
    CollectHex(lex);
  } else {
    bool seenexp = false;      // Have we seen an exponent?
    bool seendot = ch == '.';  // Have we seen a dot?
    bool seensign = false;     // Have we seen a sign char?
    bool ishex = false;        // Have seen an x or X after initial 0.
    bool isbinary = false;     // Have seen a b or B after initial 0.
    bool seenzero = ch == '0';     // Seen a zero at start.
    bool isoctal = seenzero;      // Number is octal.
    
    // We are going to use spelling as our storage, so clear it ready
    // for use.
    StringClear(&lex->spelling);
    StringAppendChar(&lex->spelling, ch);
    lex->pos++;
    if (seenzero && !CompilerCXXAtLeast(kLanguageStandardCXX14) &&
        lex->pos + 1 < lex->line.length &&
        (lex->line.value[lex->pos] == 'b' ||
         lex->line.value[lex->pos] == 'B') &&
        (lex->line.value[lex->pos + 1] == '0' ||
         lex->line.value[lex->pos + 1] == '1')) {
      LexError(lex, "Binary integer literals require C++14");
    }
    // Collect the number into spelling.  Then, when we know
    // what type of number it is, we can do the conversion to
    // binary.
    while (!SourceEof(lex->source) && lex->pos < lex->line.length) {
      ch = lex->line.value[lex->pos];
      if (seenzero && (ch == 'x' || ch == 'X')) {
        // 0x or 0X.
        ishex = true;
        isoctal = false;
      } else if (seenzero && CompilerCXXAtLeast(kLanguageStandardCXX14) &&
                 (ch == 'b' || ch == 'B')) {
        // 0b or 0B.
        isbinary = true;
        isoctal = false;
      } else if (ch == '.') {
        if (seendot) {
          // Two dots terminate number.
          break;
        }
        seendot = true;
        isoctal = false;
      } else if (!ishex && (ch == 'e' || ch == 'E')) {
        // Decimal exponent.
        if (seenexp) {
          // Already seen exponent, terminate.
          break;
        }
        seenexp = true;
        isoctal = false;
      } else if (ishex && (ch == 'p' || ch == 'P')) {
        // Binary exponent.
        if (seenexp) {
          break;
        }
        seenexp = true;
        ishex = false;      // Exponents are decimal.
      } else if (ch == '+' || ch == '-') {
        if (!seenexp || seensign) {
          // Signs can only be after exponent.
          break;
        }
        seensign = true;
      } else if (IsDigitSeparator(lex, ishex, isoctal, isbinary)) {
        lex->pos++;
        continue;
      } else if (isbinary) {
        if (ch != '0' && ch != '1') {
          break;
        }
      } else if (isoctal) {
        // Octal number.
        if (ch < '0' || ch > '7') {
          break;
        }
      } else if (ishex) {
        // Hex number.
        if (!isxdigit((unsigned char)ch)) {
          break;
        }
      } else if (!isdigit((unsigned char)ch)) {
        // Decimal number: not a digit, terminate.
        break;
      }
      StringAppendChar(&lex->spelling, ch);
      lex->pos++;
      seenzero = false;
    }
    // Terminate spelling.
    StringAppendChar(&lex->spelling, '\0');
    
    // Now we can determine the type.  If we've seen a dot
    // or exponent then we are a floating point number.
    // A suffix of ‘F’ or ‘f’ is also floating point.
    bool isfp = seendot || seenexp || toupper(CurrentChar(lex)) == 'F';
    
    // Collect the appropriate type of suffix.
    if (isfp) {
      CollectFloatingSuffix(lex);
    } else {
      CollectIntegerSuffix(lex);
    }
    CollectUserDefinedLiteralSuffix(lex);
    
    // Finally we can convert to binary using a standard library
    // function.
    if (isfp) {
      lex->fnumber = strtod(lex->spelling.value, NULL);
      lex->current_token = TOK(fnumber);
    } else {
      errno = 0;
      if (isbinary) {
        lex->number = 0;
        for (size_t i = 2; i + 1 < lex->spelling.length; i++) {
          lex->number = (lex->number << 1) | (lex->spelling.value[i] - '0');
        }
      } else {
        lex->number = strtoull(lex->spelling.value, NULL, 0);
      }
      if (lex->number == ULLONG_MAX) {
        // Possible overflow.
        if (errno == ERANGE) {
          LexError(lex, "Invalid integer literal %s", lex->spelling);
          lex->number = 0;
        }
      }
      lex->current_token = TOK(number);
    }
  }
}

// Reads another token into current_token.
void LexNextToken(Lex* lex) {
  // lex->current_token = TOK(eof);
  StringClear(&lex->ud_suffix);
  lex->literal_encoding = kLiteralEncodingNone;
  lex->literal_is_raw = false;
  LexSkipSpacesAndComments(lex);

  // Keep track of the start of the token before we read it.
  size_t token_start = lex->pos;
  int lineno = lex->source->lineno;

  // Assume token is bad.  It will be set to a valid token if possible.
  lex->current_token = TOK(bad);
  if (LexEof(lex)) {
    // End of file.
    lex->current_token = TOK(eof);
    goto record_token_location;
  }

  char ch = CurrentChar(lex);

  // Check for identifier, reserved word or wide string.
  // Wide strings (and character constants) begin with upper
  // case L followed by a quote.
  if (LexIdentifierCharByteCount(lex->line.value, lex->pos,
                                 lex->line.length, true) != 0 ||
      (lex->assembler_mode && (ch == '.' || ch == '@'))) {
    CollectIdentifierOrWide(lex);
    goto record_token_location;
  }

  // Check for number or octal(or hex) constant.
  // NOTE that a floating point number can begin with . but we need to make
  // sure we don't confuse a singular dot or ellipsis (...) here.
  // For compatibility with other assemblers, we also allow a $ to represent
  // a hex number.
  if (isdigit((unsigned char)ch) || (lex->assembler_mode && ch == '$') ||
      (ch == '.' && isdigit((unsigned char)LookaheadChar(lex)))) {
    CollectNumber(lex, ch);
    goto record_token_location;
  }

  // String literal?
  if (ch == '"') {
    CollectStringLiteral(lex);
    lex->current_token = TOK(string);
    goto record_token_location;
  }

  // Character constant?
  if (ch == '\'') {
    lex->number = CollectCharConst(lex);
    lex->current_token = TOK(charconst);
    goto record_token_location;
  }

  // Finally, check for an operator.  This will either leave the current
  // token as TOK(bad) or will set it to a valid operator token, advancing
  // pos to after the valid character sequence.
  CollectOperator(lex);

record_token_location:
  if (lex->current_token == TOK(bad)) {
    lex->pos++;
  }
  // Record the token location now that we know the start and end indexes.
  lex->current_token_location =
      NewSourceLocation(lex->source, lineno, token_start, lex->pos);
}

bool LexMatch(Lex* lex, Token token) {
  if (lex->current_token == token) {
    // Token matches, read another.
    LexNextToken(lex);
    return true;
  }
  return false;
}

bool LexMatchIdentifier(Lex* lex, String* string) {
  if (lex->current_token == TOK(identifier)) {
    // Current token is an identifier, read its spelling and another
    // token.
    StringSetString(string, &lex->spelling);
    LexNextToken(lex);
    return true;
  }
  return false;
}

bool LexLookingAt(Lex* lex, Token tok) { return lex->current_token == tok; }

// Reads another line from the input.
void LexReadLine(Lex* lex) {
  StringClear(&lex->line);
  lex->pos = 0;

  // Outer loop: terminates when we have a valid line.  Iterates on EOF
  // from a nested include file.
  while (!SourceEof(lex->source)) {
    // Reads lines until we get one that is not a preprocessor command.
    while (!SourceEof(lex->source)) {
      // Read a line into the 'line' string.  This terminates
      // at an unescaped newline character or the end of file.  It also replaces
      // trigraphs.
      SourceReadLine(lex->source, &lex->line);

      // Check for preprocessing directive.
      bool directive =
          PreprocessorParseDirective(lex->preprocessor, &lex->line);
      if (!directive) {
        // Not a preprocessor directive, therefore this is a line that should be
        // seen by the lexical analyzer.
        //
        // But check if the code has been #ifed out by the preprocessor.
        if (PreprocessorLineIsCompiledIn(lex->preprocessor)) {
          PreprocessorReplaceMacros(lex->preprocessor, &lex->line);
          break;
        }
      }

      // A preprocessing directive was read, clear current line and read
      // another.
      StringClear(&lex->line);
      lex->pos = 0;
    }

    if (SourceEof(lex->source)) {
      // If the source is nested (from an include file) we move to the
      // previous one and continue reading.
      if (lex->source->prev != NULL) {
        Source* prev = lex->source->prev;
        SourceDelete(lex->source);
        lex->source = prev;
        compiler->current_include_path_index = prev->path_index;
        continue;
      }
    }
    break;
  }
}

// Skips spaces in the input.
void LexSkipSpacesAndComments(Lex* lex) {
  while (!SourceEof(lex->source)) {
    while (!SourceEof(lex->source) && lex->pos < lex->line.length) {
      char ch = lex->line.value[lex->pos];

      // Check for a comment.
      if (ch == '/') {
        // '//' comment?
        if (lex->pos < lex->line.length &&
            lex->line.value[lex->pos + 1] == '/') {
          if (lex->assembler_mode) {
            // In assembler mode we stop when we reach a line comment
            // because the assembler will read the next line itself.
            // But we set the current position to the line length.
            lex->pos = lex->line.length;
            break;
          }
          // Single line comment, read another line.
          LexReadLine(lex);

          // And continue to skip spaces.
          continue;
        } else if (lex->pos < lex->line.length - 1 &&
                   lex->line.value[lex->pos + 1] == '*') {
          // Multi-line comment.  Read until we find the */ at the end,
          // skipping lines as we go.
          lex->pos += 2;  // Skip /*.
          lex->in_comment = true;
          bool nested_comment_warned = false;
          char prev = '\0';
          do {
            ch = GetCharInComment(lex);
            if (!nested_comment_warned && prev == '/' && ch == '*') {
              LexWarning(lex, "comment", "'/*' within block comment");
              nested_comment_warned = true;
            }
            if (prev == '*' && ch == '/') {
              break;
            }
            prev = ch;
          } while (!SourceEof(lex->source));

          lex->in_comment = false;
          // Continue to get another token.
          continue;
        }
      }

      if (!isspace((unsigned char)ch)) {
        break;
      }
      lex->pos++;
    }
    if (SourceEof(lex->source)) {
      // End of file.
      break;
    }
    if (lex->pos < lex->line.length) {
      // More characters in current line.
      break;
    }

    // In assembler mode we don't read past the end of line.
    if (lex->pos > 0 && lex->assembler_mode) {
      break;
    }

    // End of line, read another.
    LexReadLine(lex);
  }
}

bool LexEof(Lex* lex) {
  return lex->pos >= lex->line.length &&
    SourceEof(lex->source);
}

static void ReportSourceStack(Lex* lex) {
  Source* source = lex->source->prev;
  while (source != NULL) {
    ReportNote(source->filename.value, source->lineno, "Included from here");
    source = source->prev;
  }
}

void LexError(Lex* lex, const char* error, ...) {
  va_list ap;
  va_start(ap, error);
  VReportError(lex->source->filename.value, lex->source->lineno, error, ap);
  va_end(ap);
  ReportSourceStack(lex);
}

void VLexError(Lex* lex, const char* error, va_list ap) {
  VReportError(lex->source->filename.value, lex->source->lineno, error, ap);
  ReportSourceStack(lex);
}

void LexWarning(Lex* lex, const char* warn, const char* error, ...) {
  va_list ap;
  va_start(ap, error);
  VReportWarning(lex->source->filename.value, lex->source->lineno, warn, error,
                 ap);
  va_end(ap);
  ReportSourceStack(lex);
}

void VLexWarning(Lex* lex, const char* warn, const char* error, va_list ap) {
  VReportWarning(lex->source->filename.value, lex->source->lineno, warn, error,
                 ap);
  ReportSourceStack(lex);
}

// Read the arguments for an __attribute__ element.
// These consist of ((text)).  The two outer wrapping parentheses are stripped
// but any parentheses inside the attribute text are preserved (so that
// argument-bearing attributes like aligned(16) or format(printf, 1, 2) survive
// intact).
void LexReadAttributes(Lex* lex, String* attrs) {
  LexSkipSpacesAndComments(lex);
  // The function-like '(' of __attribute__ has already been consumed by the
  // tokenizer, so here bracket_count==1 corresponds to the attribute-list
  // paren and argument parens are at depth >= 2.  We strip the list paren but
  // keep argument parens so that aligned(16) / format(printf, 1, 2) survive.
  int bracket_count = 0;
  while (!LexEof(lex)) {
    if (lex->line.value[lex->pos] == '(') {
      bracket_count++;
      lex->pos++;
      if (bracket_count >= 2) {
        StringAppendChar(attrs, '(');
      }
    } else if (lex->line.value[lex->pos] == ')') {
      lex->pos++;
      bracket_count--;
      if (bracket_count == 0) {
        break;
      }
      StringAppendChar(attrs, ')');
    } else {
      StringAppendChar(attrs, GetCharInComment(lex));
    }
  }
  LexNextToken(lex);
}

void LexRewind(Lex* lex) {
  SourceRewind(lex->source);
  lex->pos = 0;
  lex->current_token = TOK(bad);
  StringSet(&lex->line, "");
}
