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

#endif /* statement_codegen_h */
