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

typedef enum {
  kLiteralEncodingNone,
  kLiteralEncodingWide,
  kLiteralEncodingUTF8,
  kLiteralEncodingUTF16,
  kLiteralEncodingUTF32,
} LiteralEncoding;

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
  // True only when current_token is `>=` left after consuming the first `>`
  // from a `>>=` token as a template closer. A source-written `>=` is a
  // relational operator and must never be mistaken for a template close.
  bool current_greatereq_is_split;

  String spelling;      // Spelling for identifier or string literal.
  String literal_spelling;  // Exact numeric spelling for C++ UDL fallback.
  int64_t number;       // Integer literal value.
  double fnumber;       // Floating point literal value.
  String suffix;        // Integer or floating suffix.
  String ud_suffix;     // C++ user-defined literal suffix.
  LiteralEncoding literal_encoding;
  bool literal_is_raw;
  
  bool preprocessor_mode;  // Running in preprocessor mode.
  bool in_comment;         // We are inside a multi-line comment.
  bool assembler_mode;     // Running in assembler mode.

  // When non-NULL, each fully macro-expanded input line is appended here as it
  // is read.  Used to record a deferred inline function body so it can be
  // re-lexed later (see LexBeginCapture / LexEndCapture).
  String* capture;
  // When set, LexReadLine reads raw lines without running the preprocessor (no
  // directive handling, no macro expansion).  Used while replaying captured
  // (already-expanded) text so it is not expanded a second time against a
  // possibly drifted macro state.
  bool suppress_preprocessing;
} Lex;

typedef struct {
  Source* source;
  SourceDevice source_device;
  fpos_t file_pos;
  size_t string_index;
  int lineno;
  uint32_t file_index;
  size_t path_index;

  String line;
  size_t pos;
  SourceLocation current_token_location;
  Token current_token;
  bool current_greatereq_is_split;
  String spelling;
  String literal_spelling;
  int64_t number;
  double fnumber;
  String suffix;
  String ud_suffix;
  LiteralEncoding literal_encoding;
  bool literal_is_raw;
  bool preprocessor_mode;
  bool in_comment;
  bool assembler_mode;
} LexCheckpoint;

// Initializes a lexical analyzer from a file.
bool LexInitFromFile(Lex* lex, const char* filename,
                     Preprocessor* preprocessor);
bool LexInitFromString(Lex* lex, const char* filename, String* string,
                       Preprocessor* preprocessor);

void LexRewind(Lex* lex);
void LexCheckpointSave(Lex* lex, LexCheckpoint* checkpoint);
void LexCheckpointRestore(Lex* lex, LexCheckpoint* checkpoint);
void LexCheckpointDestruct(LexCheckpoint* checkpoint);

// Begin/end capturing macro-expanded source text.  LexBeginCapture assumes the
// current token is the '{' that opens the text to record; it seeds `out` with
// that line (its content before the '{' replaced by spaces so column offsets are
// preserved) and every subsequent line read is appended until LexEndCapture.
void LexBeginCapture(Lex* lex, String* out);
void LexEndCapture(Lex* lex);

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

// Number of template-closing '>' characters represented by an unsplit source
// token. A source-written `>=` returns zero: it is a relational operator.
int LexClosingAngleCount(Token token);

// True when the current token begins with a template-closing angle bracket,
// including a merged `>>` / `>>=` token or the `>=` residue of splitting `>>=`.
bool LexLookingAtClosingAngle(Lex* lex);

// Consumes exactly one closing '>' when a template-argument or
// template-parameter list is being closed.  Because the lexer greedily merges
// consecutive '>' / '=' characters, the current token may be `>>`, `>>=` or
// `>=`; this splits off a single '>' and re-interprets the remainder as the
// next token (`>>` -> `>`, `>>=` -> `>=`, `>=` -> `=`).  Returns true if a '>'
// was consumed, or false (leaving the token untouched) if the current token
// does not begin with '>'.  See C++11 CWG N1757.
bool LexConsumeClosingAngle(Lex* lex);

// Returns the byte length of a valid UTF-8/ASCII identifier character at `pos`,
// or zero if the byte sequence is not valid at that identifier position.
size_t LexIdentifierCharByteCount(const char* text, size_t pos, size_t length,
                                  bool start);

void LexError(Lex* lex, const char* error, ...);
void VLexError(Lex* lex, const char* error, va_list ap);

void LexWarning(Lex* lex, const char* warn, const char* error, ...);
void VLexWarning(Lex* lex, const char* warn, const char* error, va_list ap);

void LexReadAttributes(Lex* lex, String* attrs);

#endif /* lex_h */
