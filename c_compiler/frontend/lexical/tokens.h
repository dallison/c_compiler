//
//  tokens.h
//  c_compiler
//
//  Created by David Allison on 10/26/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef tokens_h
#define tokens_h

// Tokens present in the C language.

#define TOK(t) kToken_##t

typedef enum {
  TOK(bad),       // Unknown.
  TOK(eof),       // End of file.
  
  // Atomic tokens.
  TOK(number),        // Integer constant.
  TOK(identifier),    // Identifier.
  TOK(string),        // String literal.
  TOK(string_wide),   // Wide string literal.
  TOK(charconst),     // Character constant.
  TOK(charconst_wide),
  TOK(fnumber),       // Floating point constant.
  
  // Operators, both expression and others.
  TOK(amp),
  TOK(ampeq),
  TOK(arrow),
  TOK(equal),
  TOK(bang),
  TOK(bar),
  TOK(caret),
  TOK(careteq),
  TOK(colon),
  TOK(comma),
  TOK(dot),
  TOK(ellipsis),
  TOK(equalequal),
  TOK(greater),
  TOK(greatereq),
  TOK(lbrace),
  TOK(less),
  TOK(lesseq),
  TOK(ampamp),
  TOK(barbar),
  TOK(lparen),
  TOK(lessless),
  TOK(lesslesseq),
  TOK(lsquare),
  TOK(minus),
  TOK(minuseq),
  TOK(minusminus),
  TOK(bangeq),
  TOK(bareq),
  TOK(percent),
  TOK(percenteq),
  TOK(plus),
  TOK(pluseq),
  TOK(plusplus),
  TOK(question),
  TOK(rbrace),
  TOK(rparen),
  TOK(greatergreater),
  TOK(greatergreatereq),
  TOK(rsquare),
  TOK(semicolon),
  TOK(slash),
  TOK(slasheq),
  TOK(star),
  TOK(stareq),
  TOK(tilde),

  // Keywords.
  TOK(auto),
  TOK(break),
  TOK(bool),
  TOK(case),
  TOK(char),
  TOK(complex),
  TOK(const),
  TOK(continue),
  TOK(default),
  TOK(do),
  TOK(double),
  TOK(else),
  TOK(enum),
  TOK(extern),
  TOK(false),
  TOK(float),
  TOK(for),
  TOK(goto),
  TOK(if),
  TOK(imaginary),
  TOK(inline),
  TOK(int),
  TOK(long),
  TOK(register),
  TOK(restrict),
  TOK(return),
  TOK(short),
  TOK(signed),
  TOK(sizeof),
  TOK(static),
  TOK(struct),
  TOK(switch),
  TOK(typedef),
  TOK(thread),
  TOK(union),
  TOK(unsigned),
  TOK(void),
  TOK(volatile),
  TOK(wchar_t),
  TOK(while),

  // Non-standard common extensions.
  TOK(asm),
  TOK(attribute),

  // Assembler tokens.
  TOK(hash),
} Token;

// Converts a token into a readable string.
const char* TokenName(Token tok);

#endif /* tokens_h */
