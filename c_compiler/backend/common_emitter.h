//
//  common_emitter.h
//  c_compiler_library
//
//  Created by David Allison on 11/28/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#ifndef common_emitter_h
#define common_emitter_h

#include <stdio.h>
#include "dstring.h"
#include "compiler.h"

FILE* EmitAssemblyFile(String* src_file, String* asm_file);
void EmitDataStart(FILE* fp);
void EmitStaticVariable(InitializedStaticVariable* var, FILE* fp);
void EmitBSSVariable(UninitializedStaticVariable* var, FILE* fp);
void EmitStringLiteralSection(FILE* fp);
void EmitLiteral(Literal* literal, FILE* fp);
void EmitDebug(FILE* fp);
void EmitP2Align(int alignment, FILE* fp);
void EmitTlsDataStart(FILE* fp);
void EmitTlsBSSStart(FILE* fp);
void EmitTlsBSSVariable(UninitializedStaticVariable* var, FILE* fp);
void EmitTlsVariable(InitializedStaticVariable* var, FILE* fp);
void EmitInitFiniArrayEntries(Vector* functions, bool is_fini, FILE* fp);

// The recorded init or fini functions, in the order they have to run.  A
// backend that writes no assembly gets the ordering from here instead of
// from the priority-suffixed section names the assembler would sort.
void CollectInitFiniArrayFunctions(Vector* functions, bool is_fini,
                                   Vector* out);

#endif /* common_emitter_h */
