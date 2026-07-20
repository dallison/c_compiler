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
void AppendFunctionTemplateInstantiation(Symbol* templ, Symbol* symbol);
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
Vector* CompleteAliasTemplateArguments(Symbol* alias, Vector* actuals);
void SetCXXAliasTemplatePlaceholderOrigin(Symbol* alias, TypeRecord* type);
TypeRecord* ParseCurrentClassTemplateType(TypeParser* parser, String* name);
bool CurrentClassNameMatchesTypeName(Struct* owner, String* name);

void CheckTagType(TypeParser* parser, Symbol* old, bool is_union, bool is_enum);
void AddInjectedEnumName(TypeParser* parser, Symbol* tag);

bool StructContainsTemplateParameter(Struct* str);
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

#endif /* type_internal_h */
