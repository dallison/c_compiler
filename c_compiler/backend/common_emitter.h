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
#include "asm_module.h"
#include "dstring.h"
#include "compiler.h"

FILE* EmitAssemblyFile(String* src_file, String* asm_file);
void EmitAssemblyPreamble(String* src_file, FILE* fp);
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
void EmitFunctionSection(FILE* fp, const char* func_name);
void EmitFunctionSectionToModule(AsmModule* module, const char* func_name);

// The recorded init or fini functions, in the order they have to run.  A
// backend that writes no assembly gets the ordering from here instead of
// from the priority-suffixed section names the assembler would sort.
void CollectInitFiniArrayFunctions(Vector* functions, bool is_fini,
                                   Vector* out);

bool EmitTranslationUnitContents(struct Compiler* compiler, FILE* asm_file);
bool EmitTranslationUnitRemainder(struct Compiler* compiler, FILE* asm_file);

// Structured equivalents used by programmatic backends. The FILE-based
// functions above remain the compatibility interface for legacy emitters.
void EmitAssemblyPreambleToModule(struct Compiler* compiler, AsmModule* module);
void EmitStaticVariableToModule(InitializedStaticVariable* var,
                                AsmModule* module);
void EmitBSSVariableToModule(UninitializedStaticVariable* var,
                             AsmModule* module);
void EmitLiteralToModule(Literal* literal, AsmModule* module);
void EmitTlsVariableToModule(InitializedStaticVariable* var,
                             AsmModule* module);
void EmitTlsBSSVariableToModule(UninitializedStaticVariable* var,
                                AsmModule* module);
void EmitInitFiniArrayEntriesToModule(Vector* functions, bool is_fini,
                                      AsmModule* module);
bool EmitTranslationUnitRemainderToModule(struct Compiler* compiler,
                                          AsmModule* module);

#endif /* common_emitter_h */
