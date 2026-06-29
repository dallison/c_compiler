//
//  semantics.h
//  c_compiler
//
//  Created by David Allison on 11/7/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef semantics_h
#define semantics_h

#include <stdarg.h>
#include "syntax.h"

void SemanticError(ASTNode* node, const char* format, ...);
void VSemanticError(ASTNode* node, const char* format, va_list ap);

void SemanticWarning(ASTNode* node, const char* warn, const char* format, ...);
void SemanticSymbolWarning(Symbol* symbol, const char* warn, const char* format, ...);
void VSemanticWarning(ASTNode* node, const char* warn, const char* format,
                      va_list ap);

// Emits a "note:" diagnostic.  Notes carry no severity of their own and are used
// to elaborate on the immediately preceding error or warning (for example, to
// list the candidates considered during a failed overload resolution).
void SemanticNote(ASTNode* node, const char* format, ...);
void SemanticNoteAtLocation(SourceLocation location, const char* format, ...);

void SemanticCheckScalarType(ASTNode* node);
void SemanticAnalyzeFunction(Syntax* syntax, ASTNode* node);
bool SemanticNodeIsCompilerGenerated(ASTNode* node);
void SemanticEnterCatchHandler(void);
void SemanticLeaveCatchHandler(void);
bool SemanticInCatchHandler(void);

void SemanticAnalyzeVariableDefinition(Syntax* syntax,
                                       VariableDeclarationASTNode* node);
bool SemanticDeduceAutoType(Symbol* sym, ASTNode* initializer,
                            ASTNode* diagnostic_node);

typedef enum {
  kConvertNormal,
  kConvertCast,
  kConvertContextualBool,
} ConversionContext;

// Convert a type 'from' to 'to', replacing 'from' with new ASTNode that
// contains the conversion operation.
void SemanticConvertType(ASTNode* from, TypeRecord* to, ConversionContext ctx);

void NormalConversion(ASTNode* from, TypeRecord* to);

void SemanticTypeConversionError(ASTNode* from, TypeRecord* to,
                                 const char* format);
void SemanticTypeConversionWarning(ASTNode* from, TypeRecord* to,
                                   const char* warn, const char* format);

#endif /* semantics_h */
