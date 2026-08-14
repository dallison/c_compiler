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
  TOK(arrowstar),
  TOK(equal),
  TOK(bang),
  TOK(bar),
  TOK(caret),
  TOK(careteq),
  TOK(reflect),
  TOK(colon),
  TOK(coloncolon),
  TOK(splice_close),
  TOK(comma),
  TOK(dot),
  TOK(dotstar),
  TOK(ellipsis),
  TOK(equalequal),
  TOK(greater),
  TOK(greatereq),
  TOK(spaceship),
  TOK(lbrace),
  TOK(less),
  TOK(lesseq),
  TOK(ampamp),
  TOK(barbar),
  TOK(lparen),
  TOK(lessless),
  TOK(lesslesseq),
  TOK(lsquare),
  TOK(splice_open),
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

  // C++ keywords.
  TOK(alignas),
  TOK(alignof),
  TOK(catch),
  TOK(char8_t),
  TOK(char16_t),
  TOK(char32_t),
  TOK(class),
  TOK(concept),
  TOK(consteval),
  TOK(constexpr),
  TOK(constinit),
  TOK(contract_assert),
  TOK(const_cast),
  TOK(co_await),
  TOK(co_return),
  TOK(co_yield),
  TOK(decltype),
  TOK(delete),
  TOK(dynamic_cast),
  TOK(explicit),
  TOK(export),
  TOK(friend),
  TOK(import),
  TOK(mutable),
  TOK(module),
  TOK(namespace),
  TOK(new),
  TOK(noexcept),
  TOK(nullptr),
  TOK(operator),
  TOK(private),
  TOK(protected),
  TOK(public),
  TOK(reinterpret_cast),
  TOK(requires),
  TOK(static_assert),
  TOK(static_cast),
  TOK(template),
  TOK(this),
  TOK(thread_local),
  TOK(throw),
  TOK(true),
  TOK(try),
  TOK(typeid),
  TOK(typename),
  TOK(using),
  TOK(virtual),

  // Non-standard common extensions.
  TOK(asm),
  TOK(attribute),

  // Assembler tokens.
  TOK(hash),
} Token;

// Converts a token into a readable string.
const char* TokenName(Token tok);

#endif /* tokens_h */
