//
//  expr_codegen.h
//  c_compiler
//
//  Created by David Allison on 11/21/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef expr_codegen_h
#define expr_codegen_h

#include "ast.h"
#include "codegen.h"

IRNode* GenerateExpression(Generator* gen, ASTNode* node);
IROpcode GetLoadOpcodeForType(TypeRecord* type);
IROpcode GetStoreOpcodeForType(TypeRecord* type);
// True when `cond` is an `&&` chain that expr_codegen can collapse into a
// single unsigned range compare (`x >= lo && x <= hi`, optionally with other
// conjuncts).  Used by statement condition branching to opt into value form.
bool ASTLogAndHasFoldableRange(ASTNode* cond);
// Constant evaluation: the local's bytes are indeterminate (erroneous in
// C++26) until written.
void GenerateConstexprUninitializedObjectMarker(Generator* gen,
                                                Symbol* symbol,
                                                SourceLocation location);
// Constant evaluation: on entry to a user-provided constructor, *this is
// indeterminate until its initializers and body write it.
void GenerateConstexprConstructorEntryMarker(Generator* gen,
                                             SourceLocation location);
// Constant evaluation: on entry to a union's trivial copy or move, *this takes
// on the source's active member.
void GenerateConstexprUnionCopyMarker(Generator* gen, SourceLocation location);

// Spill a scalar/pointer value into a fresh stack temporary (returns its
// address) and reload it, so it survives intervening calls.
IRNode* GeneratorSpillValueToTemp(Generator* gen, IRNode* value,
                                  TypeRecord* type);
IRNode* GeneratorReloadSpilledValue(Generator* gen, IRNode* addr,
                                    TypeRecord* type);

#endif /* expr_codegen_h */
