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
#include "reflection.h"
#include "unicode_name.h"
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

typedef struct {
  const char* spelling;
  Token token;
  LanguageStandard min_standard;
} CReservedWord;

// All reserved words with associated token values, sorted in
// alphabetic order so we can do a binary search on them.
static ReservedWord reserved_words[] = {
  {"_Bool", TOK(bool)},
  {"_Complex", TOK(complex)},
  {"_Imaginary", TOK(imaginary)},
  {"__alignof", TOK(alignof)},
  {"__alignof__", TOK(alignof)},
  {"__attribute__", TOK(attribute)},
  {"__complex", TOK(complex)},
  {"__complex__", TOK(complex)},
  {"__const", TOK(const)},
  {"__const__", TOK(const)},
  {"__declspec", TOK(declspec)},
  {"__inline", TOK(inline)},
  {"__inline__", TOK(inline)},
  {"__signed", TOK(signed)},
  {"__signed__", TOK(signed)},
  {"__thread", TOK(thread)},
  {"__typeof", TOK(typeof)},
  {"__typeof__", TOK(typeof)},
  {"__volatile", TOK(volatile)},
  {"__volatile__", TOK(volatile)},
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

// C keywords introduced after C99.  Unlike the legacy table above, entries in
// this table remain ordinary identifiers until their standard is selected.
static CReservedWord c_reserved_words[] = {
  {"_Alignas", TOK(alignas), kLanguageStandardC11},
  {"_Alignof", TOK(alignof), kLanguageStandardC11},
  {"_Atomic", TOK(atomic), kLanguageStandardC11},
  {"_BitInt", TOK(bitint), kLanguageStandardC23},
  {"_Noreturn", TOK(noreturn), kLanguageStandardC11},
  {"_Static_assert", TOK(static_assert), kLanguageStandardC11},
  {"_Thread_local", TOK(thread_local), kLanguageStandardC11},
  {"alignas", TOK(alignas), kLanguageStandardC23},
  {"alignof", TOK(alignof), kLanguageStandardC23},
  {"bool", TOK(bool), kLanguageStandardC23},
  {"constexpr", TOK(constexpr), kLanguageStandardC23},
  {"false", TOK(false), kLanguageStandardC23},
  {"nullptr", TOK(nullptr), kLanguageStandardC23},
  {"static_assert", TOK(static_assert), kLanguageStandardC23},
  {"thread_local", TOK(thread_local), kLanguageStandardC23},
  {"true", TOK(true), kLanguageStandardC23},
  {"typeof", TOK(typeof), kLanguageStandardC23},
  {"typeof_unqual", TOK(typeof_unqual), kLanguageStandardC23},
};

// C++ reserved words and alternative operator spellings.  This table is sorted
// alphabetically by spelling and is only used when a C++ -std= mode is active.
static CXXReservedWord cxx_reserved_words[] = {
  // GNU keyword aliases.  `__restrict` is not a C++ keyword, but GCC and Clang
  // accept it (and the other `__foo__` spellings) as extensions so that C
  // headers can be included from C++.  These sort before "alignas" because
  // '_' precedes 'a'.
  {"__alignof", TOK(alignof), kLanguageStandardCXX98},
  {"__alignof__", TOK(alignof), kLanguageStandardCXX98},
  {"__attribute__", TOK(attribute), kLanguageStandardCXX98},
  {"__const", TOK(const), kLanguageStandardCXX98},
  {"__const__", TOK(const), kLanguageStandardCXX98},
  {"__declspec", TOK(declspec), kLanguageStandardCXX98},
  {"__inline", TOK(inline), kLanguageStandardCXX98},
  {"__inline__", TOK(inline), kLanguageStandardCXX98},
  {"__restrict", TOK(restrict), kLanguageStandardCXX98},
  {"__restrict__", TOK(restrict), kLanguageStandardCXX98},
  {"__signed", TOK(signed), kLanguageStandardCXX98},
  {"__signed__", TOK(signed), kLanguageStandardCXX98},
  {"__thread", TOK(thread), kLanguageStandardCXX98},
  {"__typeof", TOK(typeof), kLanguageStandardCXX98},
  {"__typeof__", TOK(typeof), kLanguageStandardCXX98},
  {"__volatile", TOK(volatile), kLanguageStandardCXX98},
  {"__volatile__", TOK(volatile), kLanguageStandardCXX98},
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
  {"contract_assert", TOK(contract_assert), kLanguageStandardCXX26},
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
  // NOTE: `import` and `module` are intentionally NOT reserved words.  In C++20
  // they are context-sensitive and only act as keywords at the start of a
  // module-directive; elsewhere they are ordinary identifiers.  The parser
  // recognizes them by spelling (see ParseModule*/ParseImport* in syntax.c).
  {"inline", TOK(inline), kLanguageStandardCXX98},
  {"int", TOK(int), kLanguageStandardCXX98},
  {"long", TOK(long), kLanguageStandardCXX98},
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
  {"restrict", TOK(restrict), kLanguageStandardCXX98},
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
  {"typeof", TOK(typeof), kLanguageStandardCXX98},
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
#define NUM_C_RESERVED_WORDS() \
  (sizeof(c_reserved_words) / sizeof(CReservedWord))
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

static int CompareCReservedWord(const void* a, const void* b) {
  const CReservedWord* word1 = a;
  const CReservedWord* word2 = b;
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

  CReservedWord c_key;
  c_key.spelling = spelling;
  CReservedWord* c_value =
      bsearch(&c_key, c_reserved_words, NUM_C_RESERVED_WORDS(),
              sizeof(CReservedWord), CompareCReservedWord);
  if (c_value != NULL && CompilerCAtLeast(c_value->min_standard)) {
    *token = c_value->token;
    return true;
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

bool LexSpellingIsIdentifier(const char* spelling, size_t length) {
  if (spelling == NULL || length == 0) {
    return false;
  }
  size_t pos = 0;
  bool start = true;
  while (pos < length) {
    size_t consumed =
        LexIdentifierSourceCharByteCount(spelling, pos, length, start);
    if (consumed == 0) {
      return false;
    }
    pos += consumed;
    start = false;
  }
  Token keyword = TOK(identifier);
  return !IsReservedWord(spelling, &keyword);
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

static void ValidateUTF8SourceLine(Lex* lex) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX23)) {
    return;
  }
  for (size_t i = 0; i < lex->line.length;) {
    unsigned char ch = (unsigned char)lex->line.value[i];
    if (ch < 0x80) {
      i++;
      continue;
    }
    uint32_t codepoint;
    size_t bytes =
        DecodeUtf8(lex->line.value, i, lex->line.length, &codepoint);
    if (bytes == 0) {
      lex->pos = i;
      LexError(lex, "Invalid UTF-8 source character");
      lex->pos = 0;
      return;
    }
    i += bytes;
  }
}

static void ReadUTF8SourceLine(Lex* lex) {
  SourceReadLine(lex->source, &lex->line);
  ValidateUTF8SourceLine(lex);
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

static int AppendUTF8CodePoint(String* output, uint32_t cp);

static bool DecodeIdentifierUniversalCharacter(const char* text, size_t pos,
                                               size_t length, bool start,
                                               uint32_t* codepoint,
                                               size_t* bytes) {
  if (pos + 2 > length || text[pos] != '\\') {
    return false;
  }
  size_t cursor = pos + 1;
  char kind = text[cursor++];
  uint32_t value = 0;
  if (kind == 'N' && CompilerCXXAtLeast(kLanguageStandardCXX23) &&
      cursor < length && text[cursor] == '{') {
    size_t name_start = ++cursor;
    while (cursor < length && text[cursor] != '}') {
      unsigned char ch = (unsigned char)text[cursor];
      if (!(ch == ' ' || ch == '-' || isupper(ch) || isdigit(ch))) {
        return false;
      }
      cursor++;
    }
    if (cursor == name_start || cursor >= length ||
        !UnicodeCodePointFromName(&text[name_start], cursor - name_start,
                                  &value)) {
      return false;
    }
    cursor++;
  } else if (kind == 'u' &&
             CompilerCXXAtLeast(kLanguageStandardCXX23) &&
             cursor < length && text[cursor] == '{') {
    size_t digit_start = ++cursor;
    while (cursor < length && isxdigit((unsigned char)text[cursor])) {
      unsigned char digit = (unsigned char)text[cursor++];
      uint32_t numeric = isdigit(digit) ? digit - '0'
                                        : tolower(digit) - 'a' + 10;
      if (value > (UINT32_MAX - numeric) / 16) {
        return false;
      }
      value = value * 16 + numeric;
    }
    if (cursor == digit_start || cursor >= length || text[cursor] != '}') {
      return false;
    }
    cursor++;
  } else if (kind == 'u' || kind == 'U') {
    size_t digit_count = kind == 'U' ? 8 : 4;
    if (cursor + digit_count > length) {
      return false;
    }
    for (size_t i = 0; i < digit_count; i++) {
      unsigned char digit = (unsigned char)text[cursor++];
      if (!isxdigit(digit)) {
        return false;
      }
      value = value * 16 +
              (isdigit(digit) ? digit - '0' : tolower(digit) - 'a' + 10);
    }
  } else {
    return false;
  }
  if (value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff) ||
      !(start ? CodepointAllowedAtIdentifierStart(value)
              : CodepointAllowedInIdentifier(value))) {
    return false;
  }
  *codepoint = value;
  *bytes = cursor - pos;
  return true;
}

size_t LexIdentifierSourceCharByteCount(const char* text, size_t pos,
                                        size_t length, bool start) {
  size_t bytes = LexIdentifierCharByteCount(text, pos, length, start);
  if (bytes != 0) {
    return bytes;
  }
  uint32_t codepoint;
  return DecodeIdentifierUniversalCharacter(text, pos, length, start,
                                            &codepoint, &bytes)
             ? bytes
             : 0;
}

size_t LexAppendIdentifierSourceChar(String* output, const char* text,
                                     size_t pos, size_t length, bool start) {
  size_t bytes = LexIdentifierCharByteCount(text, pos, length, start);
  if (bytes != 0) {
    StringAppendSegment(output, &text[pos], bytes);
    return bytes;
  }
  uint32_t codepoint;
  if (!DecodeIdentifierUniversalCharacter(text, pos, length, start,
                                          &codepoint, &bytes)) {
    return 0;
  }
  AppendUTF8CodePoint(output, codepoint);
  return bytes;
}

// Perform escape processing on a char.  This handles
// backslashes inside a string literal or character constant.
static int LiteralEncodingSize(LiteralEncoding encoding) {
  switch (encoding) {
    case kLiteralEncodingUTF16:
      return 2;
    case kLiteralEncodingUTF32:
      return 4;
    case kLiteralEncodingWide:
      return compiler->wchar_size;
    default:
      return 1;
  }
}

static int EscapeChar(Lex* lex, LiteralEncoding encoding, int* size,
                      bool* universal) {
  *size = 1;
  *universal = false;
  char ch = lex->line.value[lex->pos];
  if (CompilerCXXAtLeast(kLanguageStandardCXX23) &&
      (ch == 'o' || ch == 'x' || ch == 'u') &&
      lex->pos + 1 < lex->line.length &&
      lex->line.value[lex->pos + 1] == '{') {
    int base = ch == 'o' ? 8 : 16;
    bool is_universal = ch == 'u';
    lex->pos += 2;
    uint64_t value = 0;
    size_t digits = 0;
    bool overflow = false;
    while (lex->pos < lex->line.length) {
      unsigned char digit = (unsigned char)lex->line.value[lex->pos];
      int numeric = -1;
      if (digit >= '0' && digit <= '9') {
        numeric = digit - '0';
      } else if (base == 16 && digit >= 'a' && digit <= 'f') {
        numeric = digit - 'a' + 10;
      } else if (base == 16 && digit >= 'A' && digit <= 'F') {
        numeric = digit - 'A' + 10;
      }
      if (numeric < 0 || numeric >= base) {
        break;
      }
      if (value > (UINT64_MAX - (uint64_t)numeric) / (uint64_t)base) {
        overflow = true;
      } else {
        value = value * (uint64_t)base + (uint64_t)numeric;
      }
      digits++;
      lex->pos++;
    }
    if (digits == 0) {
      LexError(lex, "Empty delimited escape sequence");
    }
    if (lex->pos >= lex->line.length ||
        lex->line.value[lex->pos] != '}') {
      LexError(lex, "Missing } in delimited escape sequence");
    } else {
      lex->pos++;
    }
    if (is_universal) {
      if (overflow || value > 0x10ffff ||
          (value >= 0xd800 && value <= 0xdfff)) {
        LexError(lex, "Invalid universal-character value");
        value = 0xfffd;
      }
      *universal = true;
    } else {
      lex->literal_has_numeric_escape = true;
      int bytes = LiteralEncodingSize(encoding);
      uint64_t maximum =
          bytes == 4 ? UINT32_MAX : ((UINT64_C(1) << (bytes * 8)) - 1);
      if (overflow || value > maximum) {
        LexError(lex,
                 "Delimited escape value is not representable in literal "
                 "encoding");
        value = 0;
      }
      *size = bytes;
    }
    return (int)value;
  }
  if (CompilerCXXAtLeast(kLanguageStandardCXX23) && ch == 'N' &&
      lex->pos + 1 < lex->line.length &&
      lex->line.value[lex->pos + 1] == '{') {
    lex->pos += 2;
    size_t name_start = lex->pos;
    bool valid_characters = true;
    while (lex->pos < lex->line.length &&
           lex->line.value[lex->pos] != '}') {
      unsigned char name_char = (unsigned char)lex->line.value[lex->pos];
      if (!(name_char == ' ' || name_char == '-' || isupper(name_char) ||
            isdigit(name_char))) {
        valid_characters = false;
      }
      lex->pos++;
    }
    size_t name_length = lex->pos - name_start;
    if (name_length == 0) {
      LexError(lex, "Empty named universal character escape");
    }
    if (!valid_characters) {
      LexError(lex, "Invalid character in named universal character escape");
    }
    bool terminated =
        lex->pos < lex->line.length && lex->line.value[lex->pos] == '}';
    if (!terminated) {
      LexError(lex, "Missing } in named universal character escape");
    } else {
      lex->pos++;
    }
    uint32_t value = 0xfffd;
    if (name_length != 0 && valid_characters &&
        !UnicodeCodePointFromName(&lex->line.value[name_start], name_length,
                                  &value)) {
      LexError(lex, "Unknown Unicode character name");
      value = 0xfffd;
    }
    *universal = true;
    return (int)value;
  }
  if (ch == 'x' || ch == 'X') {
    lex->literal_has_numeric_escape = true;
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
    uint32_t n = 0;
    int digits = ch == 'U' ? 8 : 4;
    int count = digits;
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
      LexError(lex, "A universal-character must have %d hex digits", digits);
    }
    if (n > 0x10ffff || (n >= 0xd800 && n <= 0xdfff)) {
      LexError(lex, "Invalid universal-character value");
      n = 0xfffd;
    }
    *universal = true;
    return (int)n;
  } else if (ch >= '0' && ch <= '7') {
    // Octal constant.
    lex->literal_has_numeric_escape = true;
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

static int AppendUTF8CodePoint(String* output, uint32_t cp) {
  if (cp <= 0x7f) {
    StringAppendChar(output, (char)cp);
    return 1;
  }
  if (cp <= 0x7ff) {
    StringAppendChar(output, (char)(0xc0 | (cp >> 6)));
    StringAppendChar(output, (char)(0x80 | (cp & 0x3f)));
    return 2;
  }
  if (cp <= 0xffff) {
    StringAppendChar(output, (char)(0xe0 | (cp >> 12)));
    StringAppendChar(output, (char)(0x80 | ((cp >> 6) & 0x3f)));
    StringAppendChar(output, (char)(0x80 | (cp & 0x3f)));
    return 3;
  }
  StringAppendChar(output, (char)(0xf0 | (cp >> 18)));
  StringAppendChar(output, (char)(0x80 | ((cp >> 12) & 0x3f)));
  StringAppendChar(output, (char)(0x80 | ((cp >> 6) & 0x3f)));
  StringAppendChar(output, (char)(0x80 | (cp & 0x3f)));
  return 4;
}

static int AppendUniversalCodePoint(String* output, LiteralEncoding encoding,
                                    uint32_t cp) {
  if (encoding == kLiteralEncodingUTF16 ||
      (encoding == kLiteralEncodingWide && compiler->wchar_size == 2)) {
    if (cp <= 0xffff) {
      StringAppendChar(output, cp & 0xff);
      StringAppendChar(output, (cp >> 8) & 0xff);
      return 1;
    }
    cp -= 0x10000;
    uint16_t high = 0xd800 | (uint16_t)(cp >> 10);
    uint16_t low = 0xdc00 | (uint16_t)(cp & 0x3ff);
    StringAppendChar(output, high & 0xff);
    StringAppendChar(output, (high >> 8) & 0xff);
    StringAppendChar(output, low & 0xff);
    StringAppendChar(output, (low >> 8) & 0xff);
    return 2;
  }
  if (encoding == kLiteralEncodingUTF32 ||
      encoding == kLiteralEncodingWide) {
    int size = LiteralEncodingSize(encoding);
    for (int i = 0; i < size; i++) {
      StringAppendChar(output, (cp >> (i * 8)) & 0xff);
    }
    return 1;
  }
  return AppendUTF8CodePoint(output, cp);
}

static void AppendEncodedSourceCharacter(Lex* lex, LiteralEncoding encoding,
                                         size_t start) {
  uint32_t codepoint;
  size_t bytes =
      DecodeUtf8(lex->line.value, start, lex->line.length, &codepoint);
  if (bytes == 0) {
    LexError(lex, "Invalid UTF-8 source character");
    lex->pos = start + 1;
    return;
  }
  if (encoding == kLiteralEncodingUTF16 ||
      encoding == kLiteralEncodingUTF32 ||
      encoding == kLiteralEncodingWide) {
    AppendUniversalCodePoint(&lex->spelling, encoding, codepoint);
  } else {
    StringAppendSegment(&lex->spelling, &lex->line.value[start], bytes);
  }
  lex->pos = start + bytes;
}

// Collect an integer suffix.
// Allows U, UL, ULL, L, LL, LU, LLU
// If LLU or LU then it is reversed to ULL or UL.
static void CollectIntegerSuffix(Lex* lex) {
  StringClear(&lex->suffix);
  // C23 bit-precise integer suffixes are wb/WB and the unsigned forms
  // uwb/uWB/Uwb/UWB.
  if (!CompilerIsCXX() && CompilerCAtLeast(kLanguageStandardC23)) {
    size_t start = lex->pos;
    size_t offset =
        toupper((unsigned char)lex->line.value[start]) == 'U' ? 1 : 0;
    size_t pair = start + offset;
    bool lower_pair =
        pair + 1 < lex->line.length && lex->line.value[pair] == 'w' &&
        lex->line.value[pair + 1] == 'b';
    bool upper_pair =
        pair + 1 < lex->line.length && lex->line.value[pair] == 'W' &&
        lex->line.value[pair + 1] == 'B';
    size_t length = offset + 2;
    if ((lower_pair || upper_pair) &&
        LexIdentifierCharByteCount(lex->line.value, start + length,
                                   lex->line.length, false) == 0) {
      if (offset != 0) {
        StringAppendChar(&lex->suffix, 'U');
      }
      StringAppend(&lex->suffix, "WB");
      lex->pos += length;
      return;
    }
  }
  // C++23 adds the size suffix `z`/`Z`, optionally combined with `u`/`U`
  // in either order.  Recognize the complete suffix before the C++ user-
  // defined-literal path sees it, but only when it ends the preprocessing
  // number: `1z_value` remains one user-defined literal suffix.
  if (CompilerCXXAtLeast(kLanguageStandardCXX23)) {
    size_t start = lex->pos;
    size_t length = 0;
    char first = toupper((unsigned char)lex->line.value[start]);
    char second =
        start + 1 < lex->line.length
            ? toupper((unsigned char)lex->line.value[start + 1])
            : '\0';
    if (first == 'Z') {
      length = second == 'U' ? 2 : 1;
    } else if (first == 'U' && second == 'Z') {
      length = 2;
    }
    if (length != 0 &&
        LexIdentifierCharByteCount(lex->line.value, start + length,
                                   lex->line.length, false) == 0) {
      for (size_t i = 0; i < length; i++) {
        StringAppendChar(
            &lex->suffix,
            toupper((unsigned char)lex->line.value[lex->pos++]));
      }
      return;
    }
  }
  if (CompilerCXXAtLeast(kLanguageStandardCXX11)) {
    size_t bytes = LexIdentifierCharByteCount(lex->line.value, lex->pos,
                                              lex->line.length, true);
    if (bytes != 0) {
      char first = toupper(lex->line.value[lex->pos]);
      bool starts_unsigned_suffix = first == 'U';
      bool followed_by_l =
          lex->pos + bytes < lex->line.length &&
          toupper(lex->line.value[lex->pos + bytes]) == 'L';
      if (starts_unsigned_suffix && !followed_by_l) {
        size_t next = lex->pos + bytes;
        size_t next_bytes = LexIdentifierCharByteCount(
            lex->line.value, next, lex->line.length, false);
        if (next_bytes != 0) {
          return;
        }
      }
    }
  }
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

// Collect a floating point suffix.  In C++23 this also recognizes the
// fixed-width extended suffixes f32/F32 and f64/F64.
static void CollectFloatingSuffix(Lex* lex) {
  StringClear(&lex->suffix);
  char ch = toupper(lex->line.value[lex->pos]);
  if (ch == 'F') {
    StringAppendChar(&lex->suffix, 'F');
    lex->pos++;
    if (lex->pos + 1 < lex->line.length &&
        ((lex->line.value[lex->pos] == '3' &&
          lex->line.value[lex->pos + 1] == '2') ||
         (lex->line.value[lex->pos] == '6' &&
          lex->line.value[lex->pos + 1] == '4'))) {
      StringAppendChar(&lex->suffix, lex->line.value[lex->pos]);
      StringAppendChar(&lex->suffix, lex->line.value[lex->pos + 1]);
      lex->pos += 2;
    }
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
  size_t bytes = LexIdentifierSourceCharByteCount(
      lex->line.value, lex->pos, lex->line.length, true);
  if (bytes == 0) {
    return;
  }
  bool start = true;
  while (lex->pos < lex->line.length) {
    bytes = LexAppendIdentifierSourceChar(
        &lex->ud_suffix, lex->line.value, lex->pos, lex->line.length, start);
    if (bytes == 0) {
      break;
    }
    lex->pos += bytes;
    start = false;
  }
}

static bool CharAt(Lex* lex, size_t offset, char ch) {
  return lex->pos + offset < lex->line.length &&
         lex->line.value[lex->pos + offset] == ch;
}

static void AppendRawLiteralChar(Lex* lex, LiteralEncoding encoding, char ch) {
  if (encoding == kLiteralEncodingUTF16 ||
      encoding == kLiteralEncodingUTF32 ||
      encoding == kLiteralEncodingWide) {
    AppendUniversalCodePoint(&lex->spelling, encoding, (unsigned char)ch);
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
  ReadUTF8SourceLine(lex);
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
    if ((unsigned char)ch >= 0x80) {
      AppendEncodedSourceCharacter(lex, encoding, lex->pos);
    } else {
      AppendRawLiteralChar(lex, encoding, ch);
      lex->pos++;
    }
  }
  if (!closed) {
    LexError(lex, "Unterminated raw string literal");
  }
  StringDestruct(&delimiter);
  CollectUserDefinedLiteralSuffix(lex);
}

// Collect a string literal into lex->spelling, omitting enclosing quotes.
// lex->pos is pointing at the open quote
static void CollectStringLiteral(Lex* lex, LiteralEncoding encoding) {
  lex->literal_encoding = encoding;
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
      bool universal;
      int v = EscapeChar(lex, encoding, &size, &universal);
      if (universal) {
        AppendUniversalCodePoint(&lex->spelling, encoding, (uint32_t)v);
      } else {
        if (encoding == kLiteralEncodingUTF8 && size == 1 && v > 0xff) {
          LexError(lex, "Escape value is not representable in char8_t");
        }
        for (int i = 0; i < size; i++) {
          StringAppendChar(&lex->spelling, (v >> i*8) & 0xff);
        }
      }
    } else if (ch == '"') {
      break;
    } else {
      AppendEncodedSourceCharacter(lex, encoding, lex->pos - 1);
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
      bool universal;
      int v = EscapeChar(lex, kLiteralEncodingWide, &size, &universal);
      if (universal) {
        AppendUniversalCodePoint(&lex->spelling, kLiteralEncodingWide,
                                 (uint32_t)v);
      } else {
        for (int i = 0; i < compiler->wchar_size; i++) {
          StringAppendChar(&lex->spelling, (v >> i * 8) & 0xff);
        }
      }
    } else if (ch == '"') {
      break;
    } else {
      AppendEncodedSourceCharacter(lex, kLiteralEncodingWide, lex->pos - 1);
    }
  }
  if (newline) {
    LexError(lex, "Newline in string literal");
  }
  CollectUserDefinedLiteralSuffix(lex);
}

static bool LiteralRequiresSingleUTF8CodeUnit(LiteralEncoding encoding) {
  return encoding == kLiteralEncodingUTF8 ||
         (encoding == kLiteralEncodingNone &&
          CompilerCXXAtLeast(kLanguageStandardCXX23));
}

// Collect a character constant.  The current pos is the open single quote.
// Returns the binary value of the character constant.
static int CollectCharConst(Lex* lex, LiteralEncoding encoding) {
  lex->literal_encoding = encoding;
  lex->literal_is_raw = false;
  lex->pos++;
  int value = 0;
  int nchars = 0;
  bool newline = false;
  bool utf8_width_error = false;
  while (lex->pos < lex->line.length) {
    char ch = lex->line.value[lex->pos++];
    if (ch == '\n') {
      newline = true;
      break;
    }
    if (ch == '\\') {
      int size;
      bool universal;
      int v = EscapeChar(lex, encoding, &size, &universal);
      if (universal) {
        if (encoding == kLiteralEncodingUTF16 ||
            encoding == kLiteralEncodingUTF32) {
          if (encoding == kLiteralEncodingUTF16 && (uint32_t)v > 0xffff) {
            LexError(lex,
                     "UTF-16 character literal must contain exactly one "
                     "code unit");
            utf8_width_error = true;
          }
          value = v;
          nchars++;
          continue;
        }
        String encoded;
        StringInit(&encoded, NULL);
        int units = AppendUTF8CodePoint(&encoded, (uint32_t)v);
        if (LiteralRequiresSingleUTF8CodeUnit(encoding) && units != 1) {
          LexError(
              lex, encoding == kLiteralEncodingUTF8
                       ? "UTF-8 character literal must contain exactly one "
                         "code unit"
                       : "Character literal must contain exactly one code unit");
          utf8_width_error = true;
        }
        for (int i = 0; i < units; i++) {
          value = (value << 8) | (unsigned char)encoded.value[i];
        }
        nchars += units;
        StringDestruct(&encoded);
      } else {
        if (encoding == kLiteralEncodingUTF8 && size == 1 && v > 0xff) {
          LexError(lex, "Escape value is not representable in char8_t");
          utf8_width_error = true;
        }
        if (encoding == kLiteralEncodingUTF16 ||
            encoding == kLiteralEncodingUTF32) {
          value = v;
          nchars++;
          continue;
        }
        for (int i = 0; i < size; i++) {
          value = (value << 8) | ((v >> i * 8) & 0xff);
        }
        nchars += size;
      }
    } else if (ch == '\'') {
      break;
    } else {
      size_t start = lex->pos - 1;
      if ((unsigned char)ch >= 0x80 &&
          (encoding == kLiteralEncodingUTF16 ||
           encoding == kLiteralEncodingUTF32 ||
           LiteralRequiresSingleUTF8CodeUnit(encoding))) {
        uint32_t codepoint;
        size_t bytes =
            DecodeUtf8(lex->line.value, start, lex->line.length, &codepoint);
        if (bytes == 0) {
          LexError(lex, "Invalid UTF-8 source character");
          bytes = 1;
          codepoint = 0xfffd;
        }
        if (encoding == kLiteralEncodingUTF16 ||
            encoding == kLiteralEncodingUTF32) {
          if (encoding == kLiteralEncodingUTF16 && codepoint > 0xffff) {
            LexError(lex,
                     "UTF-16 character literal must contain exactly one "
                     "code unit");
            utf8_width_error = true;
          }
          value = (int)codepoint;
          nchars++;
        } else {
          if (!utf8_width_error) {
            LexError(
                lex, encoding == kLiteralEncodingUTF8
                         ? "UTF-8 character literal must contain exactly one "
                           "code unit"
                         : "Character literal must contain exactly one "
                           "code unit");
            utf8_width_error = true;
          }
          for (size_t i = 0; i < bytes; i++) {
            value = (value << 8) |
                    (unsigned char)lex->line.value[start + i];
          }
          nchars += (int)bytes;
        }
        lex->pos = start + bytes;
      } else {
        value = (value << 8) | (unsigned char)ch;
        nchars++;
      }
    }
  }
  if (nchars > 4) {
    LexError(lex, "Max of 4 characters allowed in character constant");
  }
  if (nchars > 1) {
    if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
        encoding != kLiteralEncodingNone) {
      LexError(lex,
               "multi-character character literal cannot have an encoding "
               "prefix in C++26");
    }
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
      bool universal;
      int v = EscapeChar(lex, kLiteralEncodingWide, &size, &universal);
      if (compiler->wchar_size == 2 && (uint32_t)v > 0xffff) {
        LexError(lex,
                 "Wide character literal must contain exactly one code unit");
      }
      value = v;
      nchars ++;
    } else if (ch == '\'') {
      break;
    } else {
      size_t start = lex->pos - 1;
      uint32_t codepoint;
      size_t bytes =
          DecodeUtf8(lex->line.value, start, lex->line.length, &codepoint);
      if (bytes == 0) {
        LexError(lex, "Invalid UTF-8 source character");
        bytes = 1;
        codepoint = 0xfffd;
      }
      if (compiler->wchar_size == 2 && codepoint > 0xffff) {
        LexError(lex,
                 "Wide character literal must contain exactly one code unit");
      }
      value = (int)codepoint;
      lex->pos = start + bytes;
      nchars++;
    }
  }
  if (nchars > compiler->wchar_size) {
    LexError(lex, "Max of %d characters allowed in wide character constant", compiler->wchar_size);
  }
  if (CompilerCXXAtLeast(kLanguageStandardCXX26) && nchars > 1) {
    LexError(lex,
             "multi-character character literal cannot have an encoding "
             "prefix in C++26");
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

  if (!CompilerIsCXX() && CompilerCAtLeast(kLanguageStandardC11) &&
      CharAt(lex, 0, 'u') && CharAt(lex, 1, '8') &&
      (CharAt(lex, 2, '"') ||
       (CompilerCAtLeast(kLanguageStandardC23) && CharAt(lex, 2, '\'')))) {
    lex->pos += 2;
    if (CurrentChar(lex) == '"') {
      CollectStringLiteral(lex, kLiteralEncodingUTF8);
      lex->current_token = TOK(string);
    } else {
      lex->number = CollectCharConst(lex, kLiteralEncodingUTF8);
      lex->current_token = TOK(charconst);
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
        CollectStringLiteral(lex, encoding);
      }
      lex->current_token = encoding == kLiteralEncodingWide ? TOK(string_wide)
                                                            : TOK(string);
      return;
    }
    if (char_literal) {
      lex->pos += prefix_len;
      lex->number = CollectCharConst(lex, encoding);
      lex->current_token = TOK(charconst);
      return;
    }
  }

  // Identifier, collect into spelling.
  StringClear(&lex->spelling);
  bool identifier_start = true;
  while (lex->pos < lex->line.length) {
    char ch = lex->line.value[lex->pos];
    size_t bytes = LexAppendIdentifierSourceChar(
        &lex->spelling, lex->line.value, lex->pos, lex->line.length,
        identifier_start);
    if (bytes == 0 && lex->assembler_mode && (ch == '.' || ch == '@')) {
      bytes = 1;
      StringAppendChar(&lex->spelling, ch);
    }
    if (bytes == 0) {
      break;
    }
    lex->pos += bytes;
    identifier_start = false;
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
      lex->pos++;
      if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
          lex->line.value[lex->pos] == ':') {
        lex->current_token = TOK(splice_open);
        lex->pos++;
      } else {
        lex->current_token = TOK(lsquare);
      }
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
      if (CompilerCXXAtLeast(kLanguageStandardCXX26) && ch == '^') {
        lex->current_token = TOK(reflect);
        lex->pos++;
      } else if (ch == '=') {
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
      if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
          lex->line.value[lex->pos] == ']') {
        lex->current_token = TOK(splice_close);
        lex->pos++;
      } else if (CompilerIsCXX() && lex->line.value[lex->pos] == ':') {
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

    case '\\':
      lex->current_token = TOK(backslash);
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
  lex->current_greatereq_is_split = false;
  StringInit(&lex->line, NULL);
  StringInit(&lex->spelling, NULL);
  StringInit(&lex->literal_spelling, NULL);
  StringInit(&lex->suffix, NULL);
  StringInit(&lex->ud_suffix, NULL);
  lex->number = 0;
  lex->fnumber = 0;
  lex->literal_encoding = kLiteralEncodingNone;
  lex->literal_is_raw = false;
  lex->literal_has_numeric_escape = false;
  lex->pos = 0;
  lex->preprocessor = preprocessor;
  lex->preprocessor_mode = false;
  lex->assembler_mode = false;
  lex->in_comment = false;
  lex->capture = NULL;
  lex->suppress_preprocessing = false;
  lex->replay_active = false;
  lex->replay_tokens = NULL;
  lex->replay_index = 0;
  lex->replay_injected_value = NULL;
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
  if (lex->source->device == kSourceFromFile &&
      lex->source->from.file != NULL) {
    fgetpos(lex->source->from.file, &checkpoint->file_pos);
    checkpoint->string_index = 0;
  } else if (lex->source->device == kSourceFromString) {
    checkpoint->string_index = lex->source->from.string.index;
  } else {
    checkpoint->string_index = 0;
  }
  checkpoint->lineno = lex->source->lineno;
  checkpoint->file_index = lex->source->file_index;
  checkpoint->path_index = lex->source->path_index;

  StringInitFromSegment(&checkpoint->line, lex->line.value, lex->line.length);
  checkpoint->pos = lex->pos;
  checkpoint->current_token_location = lex->current_token_location;
  checkpoint->current_token = lex->current_token;
  checkpoint->current_greatereq_is_split = lex->current_greatereq_is_split;
  StringInitFromSegment(&checkpoint->spelling, lex->spelling.value,
                        lex->spelling.length);
  StringInitFromSegment(&checkpoint->literal_spelling,
                        lex->literal_spelling.value,
                        lex->literal_spelling.length);
  checkpoint->number = lex->number;
  checkpoint->fnumber = lex->fnumber;
  StringInitFromSegment(&checkpoint->suffix, lex->suffix.value,
                        lex->suffix.length);
  StringInitFromSegment(&checkpoint->ud_suffix, lex->ud_suffix.value,
                        lex->ud_suffix.length);
  checkpoint->literal_encoding = lex->literal_encoding;
  checkpoint->literal_is_raw = lex->literal_is_raw;
  checkpoint->literal_has_numeric_escape = lex->literal_has_numeric_escape;
  checkpoint->preprocessor_mode = lex->preprocessor_mode;
  checkpoint->in_comment = lex->in_comment;
  checkpoint->assembler_mode = lex->assembler_mode;
  checkpoint->replay_active = lex->replay_active;
  checkpoint->replay_tokens = lex->replay_tokens;
  checkpoint->replay_index = lex->replay_index;
  checkpoint->replay_injected_value = lex->replay_injected_value;
}

void LexCheckpointRestore(Lex* lex, LexCheckpoint* checkpoint) {
  lex->source = checkpoint->source;
  if (checkpoint->source_device == kSourceFromFile &&
      lex->source->from.file != NULL) {
    fsetpos(lex->source->from.file, &checkpoint->file_pos);
    clearerr(lex->source->from.file);
  } else if (checkpoint->source_device == kSourceFromString) {
    lex->source->from.string.index = checkpoint->string_index;
  }
  lex->source->lineno = checkpoint->lineno;
  lex->source->file_index = checkpoint->file_index;
  lex->source->path_index = checkpoint->path_index;

  StringSetString(&lex->line, &checkpoint->line);
  lex->pos = checkpoint->pos;
  lex->current_token_location = checkpoint->current_token_location;
  lex->current_token = checkpoint->current_token;
  lex->current_greatereq_is_split = checkpoint->current_greatereq_is_split;
  StringSetString(&lex->spelling, &checkpoint->spelling);
  StringSetString(&lex->literal_spelling, &checkpoint->literal_spelling);
  lex->number = checkpoint->number;
  lex->fnumber = checkpoint->fnumber;
  StringSetString(&lex->suffix, &checkpoint->suffix);
  StringSetString(&lex->ud_suffix, &checkpoint->ud_suffix);
  lex->literal_encoding = checkpoint->literal_encoding;
  lex->literal_is_raw = checkpoint->literal_is_raw;
  lex->literal_has_numeric_escape = checkpoint->literal_has_numeric_escape;
  lex->preprocessor_mode = checkpoint->preprocessor_mode;
  lex->in_comment = checkpoint->in_comment;
  lex->assembler_mode = checkpoint->assembler_mode;
  lex->replay_active = checkpoint->replay_active;
  lex->replay_tokens = checkpoint->replay_tokens;
  lex->replay_index = checkpoint->replay_index;
  lex->replay_injected_value = checkpoint->replay_injected_value;
}

void LexCheckpointDestruct(LexCheckpoint* checkpoint) {
  StringDestruct(&checkpoint->line);
  StringDestruct(&checkpoint->spelling);
  StringDestruct(&checkpoint->literal_spelling);
  StringDestruct(&checkpoint->suffix);
  StringDestruct(&checkpoint->ud_suffix);
}

void LexBeginCapture(Lex* lex, String* out) {
  StringClear(out);
  // The current token is the opening '{'; it ends at lex->pos, so the brace is
  // the character at lex->pos - 1.  Blank out everything on the line before the
  // brace (rather than dropping it) so column numbers in the replayed text match
  // the original source and diagnostics point at the right place.
  size_t brace_col = lex->pos > 0 ? lex->pos - 1 : 0;
  for (size_t i = 0; i < brace_col; i++) {
    StringAppendChar(out, ' ');
  }
  StringAppendSegment(out, lex->line.value + brace_col,
                      lex->line.length - brace_col);
  StringAppendChar(out, '\n');
  lex->capture = out;
}

void LexEndCapture(Lex* lex) { lex->capture = NULL; }

// Destruct a lexical analyzer.
void LexDestruct(Lex* lex) {
  if (lex->source != NULL) {
    SourceDestruct(lex->source);
    free(lex->source);
  }

  StringDestruct(&lex->line);
  StringDestruct(&lex->spelling);
  StringDestruct(&lex->literal_spelling);
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

static bool LanguageSupportsBinaryLiteralsAndDigitSeparators(void) {
  return CompilerCAtLeast(kLanguageStandardC23) ||
         CompilerCXXAtLeast(kLanguageStandardCXX14);
}

static bool IsDigitSeparator(Lex* lex, bool ishex, bool isoctal,
                             bool isbinary) {
  if (!LanguageSupportsBinaryLiteralsAndDigitSeparators() ||
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
  StringClear(&lex->literal_spelling);
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
    StringAppendChar(&lex->literal_spelling, ch);
    lex->pos++;
    if (seenzero && !LanguageSupportsBinaryLiteralsAndDigitSeparators() &&
        lex->pos + 1 < lex->line.length &&
        (lex->line.value[lex->pos] == 'b' ||
         lex->line.value[lex->pos] == 'B') &&
        (lex->line.value[lex->pos + 1] == '0' ||
         lex->line.value[lex->pos + 1] == '1')) {
      LexError(lex, "Binary integer literals require C23 or C++14");
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
      } else if (seenzero &&
                 LanguageSupportsBinaryLiteralsAndDigitSeparators() &&
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
        StringAppendChar(&lex->literal_spelling, ch);
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
      StringAppendChar(&lex->literal_spelling, ch);
      lex->pos++;
      seenzero = false;
    }
    // Terminate spelling.
    StringAppendChar(&lex->spelling, '\0');
    StringAppendChar(&lex->literal_spelling, '\0');
    
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
static bool TokenNeedsLiteralReplay(Token token) {
  return token == TOK(number) || token == TOK(fnumber) ||
         token == TOK(string) || token == TOK(string_wide) ||
         token == TOK(charconst) || token == TOK(charconst_wide);
}

static void LexApplyLiteralFieldsFromSpelling(Lex* lex,
                                              TokenSequenceToken* token) {
  if (lex->preprocessor == NULL || token->spelling.length == 0) {
    return;
  }
  String* source = NewString(token->spelling.value);
  StringAppendChar(source, '\n');
  Lex* saved_preprocessor_lex = lex->preprocessor->lex;
  Lex literal_lex;
  LexInitFromString(&literal_lex, "<token-sequence-value>", source,
                    lex->preprocessor);
  literal_lex.suppress_preprocessing = true;
  LexNextToken(&literal_lex);
  if (literal_lex.current_token == token->kind) {
    lex->number = literal_lex.number;
    lex->fnumber = literal_lex.fnumber;
    StringSetString(&lex->spelling, &literal_lex.spelling);
    StringSetString(&lex->literal_spelling, &literal_lex.literal_spelling);
    StringSetString(&lex->suffix, &literal_lex.suffix);
    StringSetString(&lex->ud_suffix, &literal_lex.ud_suffix);
    lex->literal_encoding = literal_lex.literal_encoding;
    lex->literal_is_raw = literal_lex.literal_is_raw;
    lex->literal_has_numeric_escape =
        literal_lex.literal_has_numeric_escape;
  }
  LexDestruct(&literal_lex);
  lex->preprocessor->lex = saved_preprocessor_lex;
}

static void LexApplyReplayToken(Lex* lex, TokenSequenceToken* token) {
  lex->replay_injected_value = NULL;
  lex->current_greatereq_is_split = false;
  StringClear(&lex->ud_suffix);
  lex->literal_encoding = kLiteralEncodingNone;
  lex->literal_is_raw = false;
  lex->literal_has_numeric_escape = false;
  lex->number = 0;
  lex->fnumber = 0.0;
  StringClear(&lex->suffix);
  lex->current_token_location = token->location;

  if (token->piece_kind != kTokenSequencePieceRaw) {
    lex->current_token = TOK(injected_value);
    lex->replay_injected_value = token->pseudo_value;
    StringClear(&lex->spelling);
    return;
  }

  lex->current_token = token->kind;
  StringClear(&lex->spelling);
  StringClear(&lex->literal_spelling);
  if (token->spelling.length > 0) {
    StringSetString(&lex->spelling, (String*)&token->spelling);
    StringSetString(&lex->literal_spelling, (String*)&token->spelling);
  } else if (token->kind == TOK(identifier) ||
             token->kind == TOK(number) || token->kind == TOK(fnumber) ||
             token->kind == TOK(string) || token->kind == TOK(string_wide) ||
             token->kind == TOK(charconst) || token->kind == TOK(charconst_wide)) {
    const char* name = TokenName(token->kind);
    if (name != NULL) {
      StringAppend(&lex->spelling, name);
      StringAppend(&lex->literal_spelling, name);
    }
  }

  if (TokenNeedsLiteralReplay(token->kind)) {
    LexApplyLiteralFieldsFromSpelling(lex, token);
  }

  if (lex->current_token == TOK(identifier) && lex->spelling.length > 0 &&
      !lex->preprocessor_mode && !lex->assembler_mode) {
    Token keyword = TOK(identifier);
    if (IsReservedWord(lex->spelling.value, &keyword)) {
      lex->current_token = keyword;
    }
  }
}

static void LexNextReplayToken(Lex* lex) {
  if (lex->replay_tokens == NULL ||
      lex->replay_index >= lex->replay_tokens->length) {
    lex->current_token = TOK(eof);
    lex->replay_injected_value = NULL;
    return;
  }
  TokenSequenceToken* token =
      lex->replay_tokens->value.p[lex->replay_index++];
  LexApplyReplayToken(lex, token);
}

void LexSwitchTokenReplay(Lex* lex, Vector* tokens) {
  lex->replay_active = true;
  lex->replay_tokens = tokens;
  lex->replay_index = 0;
  lex->replay_injected_value = NULL;
  LexNextReplayToken(lex);
}

void LexBeginTokenReplay(Lex* lex, Vector* tokens) {
  LexSwitchTokenReplay(lex, tokens);
}

void LexEndTokenReplay(Lex* lex) {
  lex->replay_active = false;
  lex->replay_tokens = NULL;
  lex->replay_index = 0;
  lex->replay_injected_value = NULL;
}

bool LexIsTokenReplaying(const Lex* lex) { return lex->replay_active; }

ASTNode* LexCurrentInjectedValue(const Lex* lex) {
  return lex->replay_injected_value;
}

void LexCurrentTokenSpelling(Lex* lex, String* spelling) {
  StringInit(spelling, NULL);
  if (lex->current_token == TOK(injected_value)) {
    return;
  }
  if (lex->replay_active && lex->spelling.length > 0) {
    StringSetString(spelling, &lex->spelling);
    return;
  }
  if (lex->literal_spelling.length > 0 &&
      (lex->current_token == TOK(number) ||
       lex->current_token == TOK(fnumber) ||
       lex->current_token == TOK(string) ||
       lex->current_token == TOK(string_wide) ||
       lex->current_token == TOK(charconst) ||
       lex->current_token == TOK(charconst_wide))) {
    size_t length = lex->literal_spelling.length;
    if (length > 0 && lex->literal_spelling.value[length - 1] == '\0') {
      length--;
    }
    StringAppendSegment(spelling, lex->literal_spelling.value, length);
    return;
  }
  int start = 0;
  int end = 0;
  int lineno = 0;
  const char* filename = NULL;
  DecodeSourceLocation(lex->current_token_location, &filename, &lineno, &start,
                       &end);
  if (end > start && (size_t)end <= lex->line.length) {
    StringAppendSegment(spelling, lex->line.value + start, (size_t)(end - start));
    return;
  }
  if (lex->spelling.length > 0) {
    StringSetString(spelling, &lex->spelling);
    return;
  }
  const char* name = TokenName(lex->current_token);
  if (name != NULL) {
    StringAppend(spelling, name);
  }
}

void LexNextToken(Lex* lex) {
  if (lex->replay_active) {
    LexNextReplayToken(lex);
    return;
  }
  // lex->current_token = TOK(eof);
  lex->current_greatereq_is_split = false;
  StringClear(&lex->ud_suffix);
  lex->literal_encoding = kLiteralEncodingNone;
  lex->literal_is_raw = false;
  lex->literal_has_numeric_escape = false;
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
  if (LexIdentifierSourceCharByteCount(lex->line.value, lex->pos,
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
    CollectStringLiteral(lex, kLiteralEncodingNone);
    lex->current_token = TOK(string);
    goto record_token_location;
  }

  // Character constant?
  if (ch == '\'') {
    lex->number = CollectCharConst(lex, kLiteralEncodingNone);
    lex->current_token = TOK(charconst);
    goto record_token_location;
  }

  if (ch == '\\' && lex->pos + 1 < lex->line.length &&
      (lex->line.value[lex->pos + 1] == 'u' ||
       lex->line.value[lex->pos + 1] == 'U' ||
       lex->line.value[lex->pos + 1] == 'N')) {
    LexError(
        lex,
        "Universal character name cannot name a basic character or an invalid "
        "identifier character");
    lex->pos++;
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

bool LexLookingAtStringLiteral(Lex* lex) {
  return LexLookingAt(lex, TOK(string)) ||
         LexLookingAt(lex, TOK(string_wide));
}

bool LexValidateUnevaluatedString(Lex* lex, const char* context,
                                  bool allow_user_defined_suffix) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX26)) {
    return true;
  }
  bool valid = true;
  if (!LexLookingAtStringLiteral(lex)) {
    LexError(lex, "%s requires an unevaluated string", context);
    return false;
  }
  if (lex->literal_encoding != kLiteralEncodingNone) {
    LexError(lex, "unevaluated string in %s cannot have an encoding prefix",
             context);
    valid = false;
  }
  if (!allow_user_defined_suffix && lex->ud_suffix.length != 0) {
    LexError(lex,
             "unevaluated string in %s cannot have a user-defined suffix",
             context);
    valid = false;
  }
  if (lex->literal_has_numeric_escape) {
    LexError(lex,
             "unevaluated string in %s cannot contain a numeric escape "
             "sequence",
             context);
    valid = false;
  }
  return valid;
}

int LexClosingAngleCount(Token token) {
  switch (token) {
    case TOK(greater):
      return 1;
    case TOK(greatergreater):
    case TOK(greatergreatereq):
      return 2;
    default:
      return 0;
  }
}

bool LexLookingAtClosingAngle(Lex* lex) {
  return LexClosingAngleCount(lex->current_token) != 0 ||
         (lex->current_token == TOK(greatereq) &&
          lex->current_greatereq_is_split);
}

bool LexConsumeClosingAngle(Lex* lex) {
  switch (lex->current_token) {
    case TOK(greater):
      // A plain '>' is fully consumed like any other token.
      LexNextToken(lex);
      return true;
    case TOK(greatergreater):
      // Consume the first '>' of `>>`; the trailing '>' remains as the current
      // token to close the enclosing list.  The buffer position already sits
      // past both characters, so no re-lexing is needed.
      lex->current_token = TOK(greater);
      return true;
    case TOK(greatergreatereq):
      // `>>=` -> consume one '>', leaving `>=`.
      lex->current_token = TOK(greatereq);
      lex->current_greatereq_is_split = true;
      return true;
    case TOK(greatereq):
      // `>=` -> consume the '>', leaving '='.  This only arises as the residue
      // of splitting `>>=`; a genuine `>=` never closes a template list.
      if (!lex->current_greatereq_is_split) {
        return false;
      }
      lex->current_token = TOK(equal);
      lex->current_greatereq_is_split = false;
      return true;
    default:
      return false;
  }
}

// Reads another line from the input.
void LexReadLine(Lex* lex) {
  StringClear(&lex->line);
  lex->pos = 0;

  // Replaying captured, already-expanded text (a deferred inline function
  // body): read one raw line and neither interpret preprocessing directives nor
  // expand macros.  The text was captured after macro replacement at its
  // original point, so re-running the preprocessor here would (a) re-execute any
  // directive lines and (b) expand against whatever macro state now exists,
  // which may differ from the state that was in effect when the body first
  // appeared.  There is no include nesting to unwind in a replay source.
  if (lex->suppress_preprocessing) {
    if (!SourceEof(lex->source)) {
      ReadUTF8SourceLine(lex);
    }
    return;
  }

  // Outer loop: terminates when we have a valid line.  Iterates on EOF
  // from a nested include file.
  while (!SourceEof(lex->source)) {
    // Reads lines until we get one that is not a preprocessor command.
    while (!SourceEof(lex->source)) {
      // Read a line into the 'line' string.  This terminates
      // at an unescaped newline character or the end of file.  It also replaces
      // trigraphs.
      ReadUTF8SourceLine(lex);

      // Check for preprocessing directive.
      bool directive =
          PreprocessorParseDirective(lex->preprocessor, &lex->line);
      if (directive &&
          PreprocessorDirectiveProducedOutput(lex->preprocessor)) {
        // The directive has already performed macro expansion and replaced
        // itself with ordinary source tokens (for example, #embed).  Expose
        // those tokens directly rather than expanding them a second time.
        if (lex->capture != NULL) {
          StringAppendSegment(lex->capture, lex->line.value, lex->line.length);
          StringAppendChar(lex->capture, '\n');
        }
        break;
      }
      if (!directive) {
        // Not a preprocessor directive, therefore this is a line that should be
        // seen by the lexical analyzer.
        //
        // But check if the code has been #ifed out by the preprocessor.
        if (PreprocessorLineIsCompiledIn(lex->preprocessor)) {
          PreprocessorReplaceMacros(lex->preprocessor, &lex->line);
          if (lex->capture != NULL) {
            StringAppendSegment(lex->capture, lex->line.value, lex->line.length);
            StringAppendChar(lex->capture, '\n');
          }
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
  // SourceEof() (feof) can already report true once the final line has been
  // read into lex->line while that line still holds unconsumed content: this
  // happens for a source file with no trailing newline.  Space/comment skipping
  // must therefore be driven by the buffered line position (pos < line.length),
  // not by SourceEof; the SourceEof checks below only decide whether another
  // line may be read.
  while (!SourceEof(lex->source) || lex->pos < lex->line.length) {
    while (lex->pos < lex->line.length) {
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
  bool emitted = VReportError(lex->source->filename.value,
                              lex->source->lineno, error, ap);
  va_end(ap);
  if (emitted) {
    ReportSourceStack(lex);
  }
}

void VLexError(Lex* lex, const char* error, va_list ap) {
  if (VReportError(lex->source->filename.value, lex->source->lineno, error,
                   ap)) {
    ReportSourceStack(lex);
  }
}

void LexWarning(Lex* lex, const char* warn, const char* error, ...) {
  va_list ap;
  va_start(ap, error);
  bool emitted =
      VReportWarning(lex->source->filename.value, lex->source->lineno, warn,
                     error, ap);
  va_end(ap);
  if (emitted) {
    ReportSourceStack(lex);
  }
}

void VLexWarning(Lex* lex, const char* warn, const char* error, va_list ap) {
  if (VReportWarning(lex->source->filename.value, lex->source->lineno, warn,
                     error, ap)) {
    ReportSourceStack(lex);
  }
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
  lex->current_greatereq_is_split = false;
  StringSet(&lex->line, "");
}
