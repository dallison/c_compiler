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

bool SemanticWarning(ASTNode* node, const char* warn, const char* format, ...);
void SemanticSymbolWarning(Symbol* symbol, const char* warn, const char* format, ...);
bool VSemanticWarning(ASTNode* node, const char* warn, const char* format,
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
// Analyzes |sym|'s initializer.  A decltype(auto) placeholder deduces from the
// initializer's value category, so that initializer is not folded.
ASTNode* SemanticAnalyzeDeducedInitializer(Symbol* sym, ASTNode* initializer);
bool SemanticDeduceAutoType(Symbol* sym, ASTNode* initializer,
                            ASTNode* diagnostic_node);

typedef enum {
  kConvertNormal,
  kConvertCast,
  kConvertContextualBool,
  // A normal conversion that is a direct-initialization (`T x(e)`, a
  // mem-initializer): explicit conversion functions yielding T are also
  // candidates ([over.match.conv]).
  kConvertDirectInit,
} ConversionContext;

// Convert a type 'from' to 'to', replacing 'from' with new ASTNode that
// contains the conversion operation.
void SemanticConvertType(ASTNode* from, TypeRecord* to, ConversionContext ctx);
// SemanticConvertType to a record the caller just created and does not keep;
// conversion nodes that use it hold their own references.
void SemanticConvertToNewType(ASTNode* from, TypeRecord* to,
                              ConversionContext ctx);

// Rank (0 exact, 1-3 trailing standard conversion) of the best implicit
// conversion function of class-typed `from` that reaches `to`, which may be a
// reference type; -1 when there is none or the choice is ambiguous.
int SemanticConversionFunctionRank(ASTNode* from, TypeRecord* to);

// Applies the unique non-explicit conversion function from a class-valued
// switch condition to an integral or enumeration type.
bool SemanticConvertCXXSwitchCondition(ASTNode* from);

void NormalConversion(ASTNode* from, TypeRecord* to);

void SemanticTypeConversionError(ASTNode* from, TypeRecord* to,
                                 const char* format);
void SemanticTypeConversionWarning(ASTNode* from, TypeRecord* to,
                                   const char* warn, const char* format);

// True unless the code being analyzed belongs to a template: a template
// pattern, an instantiation, or a member (or closure, or local class) of a
// class template or of one of its specializations.
bool SemanticAnalyzingNonTemplateCode(void);
// Reports `format` (given the class's name) at `node` and returns true when
// an object of `type` has incomplete class type in non-template code.  Not
// checked in templates, whose classes are instantiated whenever their
// template-id is named.  Outside template patterns it also reports, once, a
// class specialization that is complete but ill-formed
// (TypeUnreportedInvalidInstantiation); that returns false.
bool SemanticRejectIncompleteObjectType(ASTNode* node, TypeRecord* type,
                                        const char* format);

#endif /* semantics_h */
