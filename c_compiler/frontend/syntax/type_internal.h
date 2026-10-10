//
//  type_internal.h
//  c_compiler
//
//  Shared implementation declarations for the type subsystem.
//

#ifndef type_internal_h
#define type_internal_h

#include "type.h"
#include "type_class_internal.h"

struct Syntax;
struct CXXConstructorInitList;

struct ASTNode* IdentityCloneNode(struct ASTNode* node, void* data);

struct ASTNode* FunctionTemplateCloneSourceBody(struct TypeRecord* func_type);

void TypeRecordToTemplateKeyString(TypeRecord* type, String* result);
void TypeToString(Type type, String* result);
void QualifiersToString(Qualifiers quals, String* result);

CXXVirtualBaseInfo* NewCXXVirtualBaseInfo(TypeRecord* type, CXXAccess access,
                                          int vbtable_index);

TemplateParameter* TemplateParameterCopy(TemplateParameter* param);
TemplateArgument* TemplateArgumentCopy(TemplateArgument* arg);
Vector* TemplateArgumentVectorListCopy(Vector* list);
void TemplateArgumentVectorDelete(Vector* args);

ClassTemplatePartialSpecialization* NewClassTemplatePartialSpecialization(
    Symbol* tag_symbol, Vector* template_parameters, Vector* pattern_arguments);
void ClassTemplatePartialSpecializationDelete(
    ClassTemplatePartialSpecialization* partial);

CXXBaseSpecifier* NewCXXBaseSpecifier(TypeRecord* type, CXXAccess access,
                                      bool is_virtual);
void CXXBaseSpecifierDelete(CXXBaseSpecifier* base);
void CXXMemberUsingDeclarationDelete(CXXMemberUsingDeclaration* decl);
void CXXVirtualBaseInfoDelete(CXXVirtualBaseInfo* base);
void CXXVBTableInfoDelete(CXXVBTableInfo* info);
void CXXVTableInfoDelete(CXXVTableInfo* info);

bool TemplateArgumentEqual(TemplateArgument* left, TemplateArgument* right);
bool TemplateArgumentVectorEqual(Vector* left, Vector* right);

bool TemplateArgumentVectorContainsTemplateParameter(Vector* args);
bool TemplateArgumentContainsTemplateParameter(TemplateArgument* arg);
bool TemplateArgumentPatternVectorEqual(Vector* left, Vector* right);
Symbol* FindFunctionTemplateInstantiation(Symbol* templ, TypeRecord* func,
                                          Vector* args);
Symbol* FindFunctionTemplateInstantiationByAsmName(Symbol* templ,
                                                   const char* asm_name);
StructMember* FindMemberFunctionTemplateSpecialization(StructMember* first,
                                                       TypeRecord* type);
void AppendFunctionTemplateInstantiation(Symbol* templ, Symbol* symbol);
void FunctionTemplateInstantiationCacheDelete(
    struct FunctionTemplateInstantiationCache* cache);
bool DependentExpressionContainsTemplateParameter(struct ASTNode* expr);

TypeRecord* NewDecltypeReference(TypeRecord* expr_type, bool rvalue);

void ConversionOperatorName(TypeRecord* type, String* name);
TypeRecord* ParseCXXConversionType(TypeParser* parser);
void ParseFunctionPrototype(TypeParser* proto_parser, TypeRecord* func);
void ParseCXXExceptionSpecifier(TypeParser* parser, TypeRecord* func);
CXXRefQualifier ParseCXXRefQualifier(TypeParser* parser);

bool CXXAliasTemplatePatternNamesClassTemplate(Symbol* alias);
TypeRecord* InstantiateSimpleClassTemplate(TypeParser* parser, Symbol* templ,
                                           Vector* args);
TypeRecord* InstantiateAliasClassTemplate(TypeParser* parser, Symbol* alias,
                                          Vector* args);
TypeRecord* SubstituteTemplateParameters(TypeParser* parser, TypeRecord* type,
                                         Vector* args);
Vector* CompleteAliasTemplateArguments(TypeParser* parser, Symbol* alias,
                                       Vector* actuals);
void SetCXXAliasTemplatePlaceholderOrigin(Symbol* alias, TypeRecord* type);
TypeRecord* ParseCurrentClassTemplateType(TypeParser* parser, String* name);
bool CurrentClassNameMatchesTypeName(Struct* owner, String* name);
// Injected-class-name of `owner` or one of its bases (`Derived::Base`).
Symbol* FindInheritedInjectedClassName(Struct* owner, String* name);
// The type of that injected-class-name.  A base match returns the base
// specifier's type, which is the specialization actually inherited.
TypeRecord* FindInheritedInjectedClassType(Struct* owner, String* name);

void CheckTagType(TypeParser* parser, Symbol* old, bool is_union, bool is_enum);
void AddInjectedEnumName(TypeParser* parser, Symbol* tag);

bool StructContainsTemplateParameter(Struct* str);
// True when a lambda call operator's body names a template parameter of an
// enclosing template, rather than only the operator's own parameters.
bool LambdaCallOperatorBodyDependsOnEnclosingTemplate(Symbol* call_operator);
void AppendTemplateInstantiationName(String* name, Symbol* templ, Vector* args);
Vector* CompleteClassTemplateArguments(TypeParser* parser, Struct* template_struct,
                                       Vector* args);
void AddClassTemplatePartialSpecialization(TypeParser* parser, Symbol* primary,
                                           Symbol* partial_tag,
                                           Vector* pattern_args);

ClassTemplatePartialSpecialization* NewVariableTemplatePartialSpecialization(
    Vector* template_parameters, Vector* pattern_arguments,
    struct ASTNode* initializer, struct TypeRecord* type);
void AddVariableTemplatePartialSpecialization(TypeParser* parser,
                                              Symbol* primary,
                                              Vector* pattern_args,
                                              struct ASTNode* initializer,
                                              struct TypeRecord* type);

struct ASTNode* CloneCXXDefaultMemberInitializer(struct ASTNode* initializer);

/* Concrete enclosing-class template arguments for a member function.  An
 * inherited static member found through a non-template derived class
 * (`MixingHashState::combine`) still takes its arguments from the base
 * specialization that declares it (`HashStateBase<MixingHashState>`). */
Vector* MemberFunctionEnclosingClassArguments(Symbol* symbol);
// `member_args` with the enclosing-class arguments of `symbol` in front, in the
// numbering its body uses (a partial specialization's own parameters).  NULL
// when there are none, or when `keep_existing_prefix` and `member_args`
// already starts with them.  The caller deletes the result.
Vector* PrependMemberFunctionEnclosingArguments(struct Syntax* syntax,
                                                Symbol* symbol,
                                                Vector* member_args,
                                                bool keep_existing_prefix);
/* True when pointing `existing`'s member at `named` would replace a class
 * template specialization with a class that has no template arguments. */
bool CXXRetargetDropsDeclaringTemplateArguments(Struct* existing,
                                                Struct* named);

#endif /* type_internal_h */
