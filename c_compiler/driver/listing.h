//
//  listing.h
//  c_compiler
//
//  Compiler-generated interleaved listing: source, AST, IR, lowered IR
//  and assembly.  This is produced by the compiler itself, not the assembler.
//

#ifndef listing_h
#define listing_h

#include <stdbool.h>
#include <stdio.h>

struct Compiler;
struct Generator;
struct Vector;

void ListingApplyOptions(struct Compiler* compiler, struct Vector* options);
void ListingOpen(struct Compiler* compiler);
void ListingClose(struct Compiler* compiler);
void ListingEmitFunction(struct Generator* gen, void* target_code);

#endif /* listing_h */
