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
  kDaveTypeTraitIsConstructible,
  kDaveTypeTraitIsNothrowConstructible,
  kDaveTypeTraitIsConvertible,
  kDaveTypeTraitIsAssignable,
  kDaveTypeTraitIsNothrowAssignable,
  kDaveTypeTraitIsDestructible,
  kDaveTypeTraitIsNothrowDestructible,
  kDaveTypeTraitIsBaseOf,
  kDaveTypeTraitIsSwappable,
  kDaveTypeTraitIsSwappableWith,
  kDaveTypeTraitIsInvocable,
  kDaveTypeTraitIsNothrowInvocable,
  kDaveTypeTraitIsClass,
  kDaveTypeTraitIsUnion,
  kDaveTypeTraitIsEnum,
  kDaveTypeTraitIsMemberPointer,
  kDaveTypeTraitIsMemberObjectPointer,
  kDaveTypeTraitIsMemberFunctionPointer,
} DaveTypeTraitKind;

bool DaveTypeTraitEvaluateBool(Syntax* syntax, DaveTypeTraitKind kind,
                               Vector* type_args);

TypeRecord* DaveTypeTraitInvokeResultType(Syntax* syntax, Vector* type_args);

TypeRecord* DaveTypeTraitCommonType(Syntax* syntax, Vector* type_args);

bool TypeRecordIsInvokeResultPlaceholder(TypeRecord* type);

TypeRecord* TypeRecordSubstituteInvokeResultPlaceholder(Syntax* syntax,
                                                        TypeRecord* type,
                                                        Vector* args);

TypeRecord* TypeRecordNewInvokeResultPlaceholder(Vector* type_args);

TypeRecord* TypeRecordNewInvokeResultPlaceholderWithFlags(Vector* type_args,
                                                          Vector* pack_flags);

bool TypeRecordIsCommonTypePlaceholder(TypeRecord* type);

TypeRecord* TypeRecordSubstituteCommonTypePlaceholder(Syntax* syntax,
                                                      TypeRecord* type,
                                                      Vector* args);

TypeRecord* TypeRecordNewCommonTypePlaceholder(Vector* type_args);

TypeRecord* TypeRecordTryResolveTraitPlaceholder(Syntax* syntax,
                                                 TypeRecord* type);

#endif /* type_traits_semantics_h */
