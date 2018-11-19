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

#include "errors.h"
#include "vector.h"

// A reserved word, mapping a spelling to a token.
typedef struct {
  const char* spelling;
  Token token;
} ReservedWord;

// All reserved words with associated token values, sorted in
// alphabetic order so we can do a binary search on them.
static ReservedWord reserved_words[] = {
  {"_Bool", TOK(bool)},
  {"_Complex", TOK(complex)},
  {"_Imaginary", TOK(imaginary)},
  {"__attribute__", TOK(attribute)},
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

// Number of reserved words in the array.
#define NUM_RESERVED_WORDS() (sizeof(reserved_words) / sizeof(ReservedWord))

static int CompareReservedWord(const void* a, const void* b) {
  const ReservedWord* word1 = a;
  const ReservedWord* word2 = b;
  return strcmp(word1->spelling, word2->spelling);
}

// Perform a binary search on the reserved_words array (sorted in alphabetic
// order of keyword) to find the given spelling.  If found, set *token
// to the token value and return true.
static bool IsReservedWord(const char* spelling, Token* token) {
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

// Perform escape processing on a char.  This handles
// backslashes inside a string literal or character constant.
static int EscapeChar(Lex* lex, int* size) {
  *size = 1;
  char ch = lex->line.value[lex->pos];
  if (ch == 'x' || ch == 'X') {
    lex->pos++;
    int n = 0;
    while (lex->pos < lex->line.length && isxdigit(lex->line.value[lex->pos])) {
      ch = lex->line.value[lex->pos++];
      n <<= 4;
      if (isalpha(ch)) {
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
           isxdigit(lex->line.value[lex->pos])) {
      ch = lex->line.value[lex->pos++];
      n <<= 4;
      if (isalpha(ch)) {
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

// Collect a string literal into lex->spelling, omitting enclosing quotes.
// lex->pos is pointing at the open quote
static void CollectStringLiteral(Lex* lex) {
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
}

static void CollectWideStringLiteral(Lex* lex) {
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
      for (int i = 0; i < 4; i++) {
        StringAppendChar(&lex->spelling, (v >> i*8) & 0xff);
      }
    } else if (ch == '"') {
      break;
    } else {
      StringAppendChar(&lex->spelling, ch);
      for (int i = 0; i < 3; i++) {
        StringAppendChar(&lex->spelling, '\0');
      }
    }
  }
  if (newline) {
    LexError(lex, "Newline in string literal");
  }
}
// Collect a character constant.  The current pos is the open single quote.
// Returns the binary value of the character constant.
static int CollectCharConst(Lex* lex) {
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
  if (nchars == 0 || newline) {
    LexError(lex, "Newline in character constant");
  }
  return value;
}

static int CollectWideCharConst(Lex* lex) {
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
  if (nchars > 1) {
    LexError(lex, "Max of 1 character allowed in wide character constant");
  }
  if (nchars == 0 || newline) {
    LexError(lex, "Newline in character constant");
  }
  return value;
}

// Is 'ch' a valid character for an identifier?  For regular C
// mode this is alphanumeric or '_'.
// For the assembler it also includes '.' and '@'.
// If 'start' then it cannot be numeric.
bool IsIdentifierChar(Lex* lex, char ch, bool start) {
  if (start) {
    // Identifiers only start with alpha or _.
    if (isalpha(ch) || ch == '_') {
      return true;
    }
  } else if (isalnum(ch) || ch == '_') {
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
  if (CurrentChar(lex) == 'L') {
    if (LookaheadChar(lex) == '"') {
      // Wide string, collect into
      lex->pos++;
      CollectWideStringLiteral(lex);
      lex->current_token = TOK(string_wide);
      return;
    } else if (LookaheadChar(lex) == '\'') {
      // Wide char const.
      lex->pos++;
      lex->number = CollectWideCharConst(lex);
      lex->current_token = TOK(charconst_wide);
      return;
    }
  }
  // Identifier, collect into spelling.
  StringClear(&lex->spelling);
  while (lex->pos < lex->line.length) {
    char ch = lex->line.value[lex->pos];
    if (!IsIdentifierChar(lex, ch, false)) {
      break;
    }
    StringAppendChar(&lex->spelling, ch);
    lex->pos++;
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
        lex->current_token = TOK(arrow);
        lex->pos++;
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
      if (ch == '=') {
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
      } else {
        lex->current_token = TOK(dot);
      }
      break;

    case '?':
      lex->current_token = TOK(question);
      lex->pos++;
      break;
    case ':':
      lex->current_token = TOK(colon);
      lex->pos++;
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

static void LexInitCommon(Lex* lex, Preprocessor* preprocessor) {
  lex->current_token = TOK(bad);
  StringInit(&lex->line, NULL);
  StringInit(&lex->spelling, NULL);
  StringInit(&lex->suffix, NULL);
  lex->number = 0;
  lex->fnumber = 0;
  lex->pos = 0;
  lex->preprocessor = preprocessor;
  lex->preprocessor_mode = false;
  lex->assembler_mode = false;
  lex->in_comment = false;
  preprocessor->lex = lex;
}

// Initialize a lexical analyzer from a file.
bool LexInitFromFile(Lex* lex, const char* filename,
                     Preprocessor* preprocessor) {
  FILE* in = fopen(filename, "r");
  if (in == NULL) {
    return false;
  }
  lex->source = NewSourceFromFile(filename, in);
  LexInitCommon(lex, preprocessor);
  return true;
}

bool LexInitFromString(Lex* lex, const char* filename, String* code,
                       Preprocessor* preprocessor) {
  lex->source = NewSourceFromString(filename, code);
  LexInitCommon(lex, preprocessor);
  lex->source->path_index = preprocessor->lex->source->path_index;
  return true;
}

// Destruct a lexical analyzer.
void LexDestruct(Lex* lex) {
  if (lex->source != NULL) {
    SourceDestruct(lex->source);
    free(lex->source);
  }

  StringDestruct(&lex->line);
  StringDestruct(&lex->spelling);
}

// Collects a hexadecimal or octal number.
static void CollectHexOrOctal(Lex* lex) {
  int64_t number = 0;
  lex->pos++;
  if (lex->pos < lex->line.length &&
      toupper(lex->line.value[lex->pos]) == 'X') {
    // Collect hex number and convert to binary.
    lex->pos++;
    while (lex->pos < lex->line.length &&
           isxdigit(lex->line.value[lex->pos])) {
      char ch = lex->line.value[lex->pos++];
      number <<= 4;
      if (isalpha(ch)) {
        number |= tolower(ch) - 'a' + 10;
      } else {
        number |= ch - '0';
      }
    }
  } else {
    // Octal number, convert to binary.
    while (lex->pos < lex->line.length &&
           (lex->line.value[lex->pos] >= '0' &&
            lex->line.value[lex->pos] <= '7')) {
             number = (number << 3) | lex->line.value[lex->pos++] - '0';
           }
  }
  lex->number = number;
  CollectIntegerSuffix(lex);
  lex->current_token = TOK(number);
}

static void CollectNumber(Lex* lex, char ch) {
  // If the number begins with a 0 then it is either an octal
  // or hex number.  If the 0 is followed immediately by an x or X
  // then it is in hex.
  if (ch == '0' && LookaheadChar(lex) != '.') {
    CollectHexOrOctal(lex);
  } else {
    bool seenexp = false;      // Have we seen an exponent?
    bool seendot = ch == '.';  // Have we seen a dot?
    bool seensign = false;     // Have we seen a sign char?
    
    // We are going to use spelling as our storage, so clear it ready
    // for use.
    StringClear(&lex->spelling);
    StringAppendChar(&lex->spelling, ch);
    lex->pos++;
    
    // Collect the number into spelling.  Then, when we know
    // what type of number it is, we can do the conversion to
    // binary.
    while (!SourceEof(lex->source) && lex->pos < lex->line.length) {
      ch = lex->line.value[lex->pos];
      if (ch == '.') {
        if (seendot) {
          // Two dots terminate number.
          break;
        }
        seendot = true;
      } else if (ch == 'e' || ch == 'E') {
        if (seenexp) {
          // Already seen exponent, terminate.
          break;
        }
        seenexp = true;
      } else if (ch == '+' || ch == '-') {
        if (!seenexp || seensign) {
          // Signs can only be after exponent.
          break;
        }
        seensign = true;
      } else if (!isdigit(ch)) {
        // Not a digit, termnate.
        break;
      }
      StringAppendChar(&lex->spelling, ch);
      lex->pos++;
    }
    
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
    
    // Finally we can convert to binary using a standard library
    // function.
    if (isfp) {
      lex->fnumber = strtod(lex->spelling.value, NULL);
      lex->current_token = TOK(fnumber);
    } else {
      lex->number = strtoll(lex->spelling.value, NULL, 10);
      lex->current_token = TOK(number);
    }
  }
}

// Reads another token into current_token.
void LexNextToken(Lex* lex) {
  // lex->current_token = TOK(eof);
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
  if (IsIdentifierChar(lex, ch, true)) {
    CollectIdentifierOrWide(lex);
    goto record_token_location;
  }

  // Check for number or octal(or hex) constant.
  // NOTE that a floating point number can begin with . but we need to make
  // sure we don't confuse a singular dot or ellipsis (...) here.
  if (isdigit(ch) || (ch == '.' && isdigit(LookaheadChar(lex)))) {
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
          do {
            do {
              ch = GetCharInComment(lex);
            } while (ch != '*');
            ch = GetCharInComment(lex);
          } while (ch != '/');

          lex->in_comment = false;
          // Continue to get another token.
          continue;
        }
      }

      if (!isspace(ch)) {
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

bool LexEof(Lex* lex) { return SourceEof(lex->source); }

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
// These consist of ((text)).
void LexReadAttributes(Lex* lex, String* attrs) {
  LexSkipSpacesAndComments(lex);
  int bracket_count = 0;
  while (!LexEof(lex)) {
    if (lex->line.value[lex->pos] == '(') {
      bracket_count++;
      lex->pos++;
    } else if (lex->line.value[lex->pos] == ')') {
      lex->pos++;
      bracket_count--;
      if (bracket_count == 0) {
        break;
      }
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
