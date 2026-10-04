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
  Symbol* pattern;  // The class template's member declaration.
} PendingMemberBody;

// A member of a class template specialization instantiated before the
// template's out-of-class definition of it was parsed.
typedef struct {
  Symbol* symbol;
  Symbol* pattern;
  Struct* owner;
  Struct* substitution_source;
  Vector* args;  // Owned.
} LateMemberBody;

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

typedef struct FunctionInstantiationInProgress {
  Symbol* symbol;
  struct FunctionInstantiationInProgress* next;
} FunctionInstantiationInProgress;

bool FunctionTemplateInstantiationInProgress(Symbol* symbol);
void PushFunctionInstantiationInProgress(
    FunctionInstantiationInProgress* node, Symbol* symbol);
void PopFunctionInstantiationInProgress(
    FunctionInstantiationInProgress* node);

void RewriteTemplateBodyIdentifiers(struct ASTNode* node, Map* symbol_map);
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
bool FindPackExpansionInExpression(struct ASTNode* expr, Vector* args,
                                   int* pack_index, size_t* pack_length);
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
bool StructIsFunctionTemplateLocalClass(Struct* str);
bool CallActualsStillContainPackExpansion(struct ASTNode* call);
bool PendingTemplateInstantiationHasAsmName(const char* asm_name);

TypeRecord* InstantiateSimpleClassTemplate(TypeParser* parser, Symbol* templ,
                                           Vector* args);
/* `str` is a class-template specialization whose members are not substituted
 * yet.  Return the type of typedef `name` by substituting the pattern (or, if
 * the pattern only inherits that typedef, its base) with this specialization's
 * arguments.  Returns NULL when the pattern does not provide `name`. */
TypeRecord* ResolveInProgressClassMemberType(TypeParser* parser, Struct* str,
                                             String* name);
TypeRecord* InstantiateAliasClassTemplate(TypeParser* parser, Symbol* alias,
                                          Vector* args);
bool CXXAliasTemplatePatternNamesClassTemplate(Symbol* alias);
bool CXXAliasTemplateIsDeducible(Symbol* alias);
Vector* CompleteAliasTemplateArguments(TypeParser* parser, Symbol* alias,
                                       Vector* actuals);
Vector* MemberAliasPatternArguments(TypeParser* parser, Symbol* alias,
                                    Vector* alias_args);
/* Fold an expanded pack (`Box<char, int>` stored as `[char, int]`) back into
 * the class template's declared shape (`[pack{char, int}]`).  Returns a new
 * vector, or NULL when `expanded` is already in that shape. */
Vector* RegroupExpandedClassTemplateArguments(Symbol* class_template,
                                              Vector* expanded);
void SetCXXAliasTemplatePlaceholderOrigin(Symbol* alias, TypeRecord* type);

TypeRecord* SubstituteNestedStructTemplateParameters(TypeParser* parser,
                                                     TypeRecord* type,
                                                     Vector* args);
StructMember* InstantiateTemplateMemberFunction(TypeParser* parser, Struct* owner,
                                                StructMember* member,
                                                Vector* args, Vector* pending);

void MaxTemplateParameterIndexInArgument(TemplateArgument* arg, int* max_index);
/* A named use of an alias template whose pattern is a dependent decltype; its
 * operand is in the alias's parameter space. */
bool TypeIsDecltypeAliasTemplateId(TypeRecord* type);
bool AliasTemplateArgumentIsPackExpansion(TemplateArgument* arg, int* pack_index,
                                          TemplateParameterKind* kind);
/* `CloneDependentDecltypeNode` temporarily clears `dependent_decltype_expr`
 * on the operand's type so cloning that operand does not recurse into itself.
 * A nested instantiation of the function whose trailing return type *is* that
 * type would otherwise treat the missing expression as a plain `auto` return
 * and instantiate the body (`GetData` for an array). */
void TypeNoteDetachedDependentDecltype(struct TypeRecord* type);
void TypeForgetDetachedDependentDecltype(struct TypeRecord* type);
bool TypeIsDetachedDependentDecltype(struct TypeRecord* type);
struct ASTNode* CloneDependentDecltypeNode(struct ASTNode* node, void* data);
struct ASTNode* CloneTemplateFunctionBodyNode(struct ASTNode* node, void* data);
struct ASTNode* CloneTemplateFunctionBody(TypeParser* parser, TypeRecord* from,
                                          TypeRecord* to, Vector* args);
void QueueTemplateMemberFunctionDefinitionImpl(Symbol* symbol,
                                             Symbol* template_definition,
                                             TypeParser* parser, Vector* args,
                                             bool allow_lazy);
// When `owner` is an instantiation of one of its template's partial
// specializations, the number of that partial's own parameters; its members
// are numbered by those rather than by the arguments recorded on `owner`'s
// type.  -1 for an instantiation of the primary template.
int StructPartialSpecializationParameterCount(struct Syntax* syntax,
                                              Struct* owner);
// The partial specialization's own bindings for `owner` (caller deletes), or
// NULL for an instantiation of the primary template.
Vector* StructPartialSpecializationPatternArguments(struct Syntax* syntax,
                                                    Struct* owner);
void CloneInstantiatedMemberFunctionBody(TypeParser* parser, Struct* owner,
                                         Symbol* symbol,
                                         Symbol* template_definition,
                                         Struct* substitution_source,
                                         Vector* args);
void ReanalyzeDeferredDependentAssignments(TypeParser* parser, TypeRecord* type);

TemplateArgument* NewEmptyPackTemplateArgument(TemplateParameterKind kind);

#endif /* type_template_internal_h */
