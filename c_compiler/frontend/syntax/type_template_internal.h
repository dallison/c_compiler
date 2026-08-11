//
//  type_template_internal.h
//  c_compiler
//
//  Cross-module declarations for the template subsystem.
//

#ifndef type_template_internal_h
#define type_template_internal_h

#include "type.h"
#include "map.h"

struct Syntax;
struct ASTNode;

typedef struct {
  Symbol* origin;
  Vector* args;
} TemplateIdSubstitutionInfo;

typedef struct {
  Symbol* symbol;
  Symbol* template_definition;
  Struct* substitution_source;
} PendingMemberBody;

typedef struct {
  Map symbol_map;
  Map pack_symbol_map;
  TypeParser* parser;
  Vector* args;
  TypeRecord* from_func;
  TypeRecord* to_func;
  int rebase_template_parameter_base;
  struct Struct* from_owner;
  struct Struct* to_owner;
  struct Struct* substitution_source;
  struct Struct* substitution_target;
} TemplateFunctionBodyClone;

struct ASTNode* IdentityCloneNode(struct ASTNode* node, void* data);
void DeleteMappedVector(MapKeyValue* kv);

TypeRecord* SubstituteTemplateParameters(TypeParser* parser, TypeRecord* type,
                                         Vector* args);
Vector* SubstituteTemplateArgumentVector(TypeParser* parser, Vector* template_args,
                                         Vector* args, int rebase_base);
Vector* SubstituteTemplateArgumentVectorForTypes(TypeParser* parser,
                                                 Vector* template_args,
                                                 Vector* args);
struct ASTNode* CloneDependentExpressionWithArgs(TypeParser* parser,
                                               struct ASTNode* expr,
                                               Vector* args);
bool TryFoldDependentTemplateArgument(TypeParser* parser, struct ASTNode* expr,
                                      Vector* args, int64_t* out);
TemplateArgument* NewSubstitutedTemplateArgument(TypeParser* parser,
                                                 TemplateArgument* arg,
                                                 Vector* args);

bool FindPackExpansionInType(TypeRecord* type, Vector* args, int* pack_index,
                             size_t* pack_length);
bool FindPackExpansionInTemplateArgument(TemplateArgument* arg, Vector* args,
                                         int* pack_index,
                                         size_t* pack_length);
Vector* TemplateArgumentVectorCopyWithPackElement(Vector* args, int pack_index,
                                                  TemplateArgument* element);
void SubstituteDependentSymbolValue(Symbol* symbol, Vector* args);
void SubstituteDependentSymbolAlignment(TypeParser* parser, Symbol* symbol,
                                        Vector* args);
void SubstituteStaticMemberInitializerValue(TypeParser* parser, Symbol* symbol,
                                            struct ASTNode* initializer,
                                            Vector* args);
void LambdaCapturePackElementName(String* name, const char* base, size_t index);
bool LambdaCapturePackElementMatches(const char* name, const char* base);
int FirstTemplateParameterIndexInType(TypeRecord* type);
TypeRecord* SubstituteTemplateParametersForPackElement(TypeParser* parser,
                                                       TypeRecord* type,
                                                       Vector* args,
                                                       int pack_index,
                                                       size_t element_index);
void RebaseTemplateParameterIndices(TypeRecord* type, int base);
void RebaseTemplateArgumentParameterIndices(TemplateArgument* arg, int base);
void AppendSubstitutedFormalParameter(TypeParser* parser, Vector* out,
                                      Symbol* formal, Vector* args,
                                      int rebase_base);

const char* CXXConstructorNameForRecord(TypeRecord* type);
void DeleteStringVector(Vector* strings);
Vector* SplitDependentMemberPath(String* path);
bool StructContainsTemplateParameter(Struct* str);
bool CallActualsStillContainPackExpansion(struct ASTNode* call);
bool PendingTemplateInstantiationHasAsmName(const char* asm_name);

TypeRecord* InstantiateSimpleClassTemplate(TypeParser* parser, Symbol* templ,
                                           Vector* args);
TypeRecord* InstantiateAliasClassTemplate(TypeParser* parser, Symbol* alias,
                                          Vector* args);
bool CXXAliasTemplatePatternNamesClassTemplate(Symbol* alias);
Vector* CompleteAliasTemplateArguments(Symbol* alias, Vector* actuals);
void SetCXXAliasTemplatePlaceholderOrigin(Symbol* alias, TypeRecord* type);

TypeRecord* SubstituteNestedStructTemplateParameters(TypeParser* parser,
                                                     TypeRecord* type,
                                                     Vector* args);
StructMember* InstantiateTemplateMemberFunction(TypeParser* parser, Struct* owner,
                                                StructMember* member,
                                                Vector* args, Vector* pending);

void MaxTemplateParameterIndexInArgument(TemplateArgument* arg, int* max_index);
bool AliasTemplateArgumentIsPackExpansion(TemplateArgument* arg, int* pack_index,
                                          TemplateParameterKind* kind);
struct ASTNode* CloneDependentDecltypeNode(struct ASTNode* node, void* data);
struct ASTNode* CloneTemplateFunctionBodyNode(struct ASTNode* node, void* data);
struct ASTNode* CloneTemplateFunctionBody(TypeParser* parser, TypeRecord* from,
                                          TypeRecord* to, Vector* args);
void QueueTemplateMemberFunctionDefinitionImpl(Symbol* symbol,
                                             Symbol* template_definition,
                                             TypeParser* parser, Vector* args,
                                             bool allow_lazy);
void CloneInstantiatedMemberFunctionBody(TypeParser* parser, Struct* owner,
                                         Symbol* symbol,
                                         Symbol* template_definition,
                                         Struct* substitution_source,
                                         Vector* args);
void ReanalyzeDeferredDependentAssignments(TypeParser* parser, TypeRecord* type);

TemplateArgument* NewEmptyPackTemplateArgument(TemplateParameterKind kind);

#endif /* type_template_internal_h */
