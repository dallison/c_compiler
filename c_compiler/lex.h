//
//  lex.h
//  c_compiler
//
//  Created by David Allison on 10/26/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef lex_h
#define lex_h

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "dstring.h"
#include "preprocessor.h"
#include "source.h"
#include "tokens.h"
#include "vector.h"

// The lexical analyzer.  This struct contains all the data
// necessary to convert the input source code to the tokens
// in the C language.
//
// The preprocessor is defined by the language standard as having its
// own expression syntax for #if directives.  This is slightly different
// from the C language syntax and semantics.  In some compilers, the
// preprocessor is a separate program that runs over the source code
// before the compiler runs.  This means that it would have its own
// lexical and syntax analyzers.  In this compiler we run it in a separate
// phase and we can reuse the lexical and syntax analyzers for both the
// preprocessor and compiler proper.  If the 'preprocessor_mode'
// member is set to true then we allow preprocessor expressions.
typedef struct Lex {
  Source* source;  // Current input source.
  String line;     // Input line.
  size_t pos;      // Next position in line.

  // Location of current token.
  SourceLocation current_token_location;

  Preprocessor* preprocessor;

  Token current_token;  // Current token.

  String spelling;      // Spelling for identifier or string literal.
  int64_t number;       // Integer literal value.
  double fnumber;       // Floating point literal value.
  String suffix;        // Integer or floating suffix.
  
  bool preprocessor_mode;  // Running in preprocessor mode.
  bool in_comment;         // We are inside a multi-line comment.
  bool assembler_mode;     // Running in assembler mode.
} Lex;

// Initializes a lexical analyzer from a file.
bool LexInitFromFile(Lex* lex, const char* filename,
                     Preprocessor* preprocessor);
bool LexInitFromString(Lex* lex, const char* filename, String* string,
                       Preprocessor* preprocessor);

void LexRewind(Lex* lex);

// Destructs a lexical analyzer but does not free the memory.
void LexDestruct(Lex* lex);

// Reads another token into current_token.
void LexNextToken(Lex* lex);

// Reads another line from the input.
void LexReadLine(Lex* lex);

// Skips spaces in the input.
void LexSkipSpacesAndComments(Lex* lex);

// End of file?
bool LexEof(Lex* lex);

// Does the current token match that given?  If so, move on to the next
// token and return true.
bool LexMatch(Lex* lex, Token token);

// Is the current token an identifier?  If so, get the identifier's
// spelling into the given string, move on to the next token and
// return true.
bool LexMatchIdentifier(Lex* lex, String* spelling);

bool LexLookingAt(Lex* lex, Token tok);

void LexError(Lex* lex, const char* error, ...);
void VLexError(Lex* lex, const char* error, va_list ap);

void LexWarning(Lex* lex, const char* warn, const char* error, ...);
void VLexWarning(Lex* lex, const char* warn, const char* error, va_list ap);

void LexReadAttributes(Lex* lex, String* attrs);

#endif /* lex_h */
