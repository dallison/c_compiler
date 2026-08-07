//
//  type_traits_semantics.h
//  c_compiler
//
//  Compile-time type trait evaluation for DaveCC builtins.
//

#ifndef type_traits_semantics_h
#define type_traits_semantics_h

#include "syntax.h"
#include "type.h"

typedef enum {
  kCXXTypeTraitIsConstructible,
  kCXXTypeTraitIsNothrowConstructible,
  kCXXTypeTraitIsConvertible,
  kCXXTypeTraitIsAssignable,
  kCXXTypeTraitIsNothrowAssignable,
  kCXXTypeTraitIsDestructible,
  kCXXTypeTraitIsNothrowDestructible,
  kCXXTypeTraitIsTriviallyCopyable,
  kCXXTypeTraitIsBaseOf,
  kCXXTypeTraitIsSwappable,
  kCXXTypeTraitIsSwappableWith,
  kCXXTypeTraitIsInvocable,
  kCXXTypeTraitIsNothrowInvocable,
  kCXXTypeTraitIsClass,
  kCXXTypeTraitIsUnion,
  kCXXTypeTraitIsEnum,
  kCXXTypeTraitIsMemberPointer,
  kCXXTypeTraitIsMemberObjectPointer,
  kCXXTypeTraitIsMemberFunctionPointer,
  kCXXTypeTraitMemberPointerDirectObject,
} CXXTypeTraitKind;

bool CXXTypeTraitEvaluateBool(Syntax* syntax, CXXTypeTraitKind kind,
                              Vector* type_args);

TypeRecord* CXXTypeTraitInvokeResultType(Syntax* syntax, Vector* type_args);

TypeRecord* CXXTypeTraitCommonType(Syntax* syntax, Vector* type_args);

bool TypeRecordIsInvokeResultPlaceholder(TypeRecord* type);

TypeRecord* TypeRecordSubstituteInvokeResultPlaceholder(TypeParser* parser,
                                                        TypeRecord* type,
                                                        Vector* args);

TypeRecord* TypeRecordNewInvokeResultPlaceholder(Vector* type_args);

TypeRecord* TypeRecordNewInvokeResultPlaceholderWithFlags(Vector* type_args,
                                                          Vector* pack_flags);

bool TypeRecordIsCommonTypePlaceholder(TypeRecord* type);

TypeRecord* TypeRecordSubstituteCommonTypePlaceholder(TypeParser* parser,
                                                      TypeRecord* type,
                                                      Vector* args);

TypeRecord* TypeRecordNewCommonTypePlaceholder(Vector* type_args);

TypeRecord* TypeRecordTryResolveTraitPlaceholder(Syntax* syntax,
                                                 TypeRecord* type);

#endif /* type_traits_semantics_h */
