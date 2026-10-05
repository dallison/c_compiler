//
//  statement_codegen.h
//  c_compiler
//
//  Created by David Allison on 11/21/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef statement_codegen_h
#define statement_codegen_h

#include "ast.h"
#include "codegen.h"

void GenerateStatement(Generator* gen, ASTNode* node);
IRNode* GenerateVLASize(Generator* gen, TypeRecord* type);
void GenerateFunctionContractAssertions(Generator* gen,
                                        ContractAssertionKind kind);

// Runtime terminate guard for noexcept functions (see statement_codegen.c).
// The labels bracket the guarded region (the whole function body); `active`
// records whether a guard is actually needed for this function.
typedef struct {
  bool active;
  IRNode* try_start;
  IRNode* try_end;
} NoexceptTerminateGuard;

void GenerateNoexceptGuardEnter(Generator* gen, NoexceptTerminateGuard* guard);
void GenerateNoexceptGuardLeave(Generator* gen, NoexceptTerminateGuard* guard);
void GenerateNoexceptGuardTerminate(Generator* gen,
                                    NoexceptTerminateGuard* guard);

// Emits the C++ scope-exit cleanup landing pads scheduled while generating the
// body; call after the return path.  FreeCleanupPads releases their bookkeeping
// and must be called when the generator is torn down.
void GenerateCleanupLandingPads(Generator* gen);
void FreeCleanupPads(Generator* gen);

// `left, temporary.~T()` from the end of a full-expression: generates both and
// guards the destructor with a flag recording whether the temporary was
// constructed.  GenerateNoteTemporaryConstruction sets that flag; call it after
// generating any expression while gen->temporary_cleanups is non-empty.
IRNode* GenerateTemporaryCleanupComma(Generator* gen, BinaryASTNode* node);
void GenerateNoteTemporaryConstruction(Generator* gen, ASTNode* node);

#endif /* statement_codegen_h */
