//
//  reflection.h
//  c_compiler
//

#ifndef reflection_h
#define reflection_h

#include "tokens.h"
#include "type.h"
#include "symbol_table.h"
#include "vector.h"

struct ASTNode;
struct TemplateArgument;

typedef enum {
  kReflectionInvalid,
  kReflectionGlobalNamespace,
  kReflectionNamespace,
  kReflectionType,
  kReflectionTypeAlias,
  kReflectionVariable,
  kReflectionFunction,
  kReflectionTemplate,
  kReflectionEnumerator,
  kReflectionDataMember,
  kReflectionBase,
  kReflectionValue,
  kReflectionObject,
  kReflectionStructuredBinding,
  kReflectionFunctionParameter,
  kReflectionAnnotation,
  kReflectionClassMember,
  kReflectionUnnamedBitField,
  kReflectionClassTemplate,
  kReflectionFunctionTemplate,
  kReflectionVariableTemplate,
  kReflectionAliasTemplate,
  kReflectionConcept,
  kReflectionNamespaceAlias,
  kReflectionDataMemberDescription,
  kReflectionEnumeratorDescription,
  kReflectionTokenSequence,
} ReflectionEntityKind;

typedef enum {
  kTokenSequencePieceRaw,
  // An evaluated typed pseudo-token.  Replay exposes its AST value directly
  // to the expression parser without detokenizing it.
  kTokenSequencePieceValue,
  kTokenSequencePieceTokenInterpolation,
  kTokenSequencePieceIdentifierInterpolation,
  kTokenSequencePieceTokensInterpolation,
} TokenSequencePieceKind;

typedef struct TokenSequenceToken {
  Token kind;
  String spelling;
  SourceLocation location;
  TokenSequencePieceKind piece_kind;
  struct ASTNode* pseudo_value;
} TokenSequenceToken;

struct Attribute;

typedef struct ReflectionDataMemberSpec {
  TypeRecord* member_type;
  String name;
  size_t alignment;
  size_t bit_width;
  bool no_unique_address;
  bool has_name;
  bool has_alignment;
  bool has_bit_width;
  Vector annotations;
} ReflectionDataMemberSpec;

typedef struct ReflectionEnumeratorSpec {
  String name;
  int64_t value;
  bool has_value;
  bool has_name;
  Vector attributes;
  Vector annotations;
} ReflectionEnumeratorSpec;

typedef struct ReflectionValue {
  ReflectionEntityKind kind;
  TypeRecord* reflected_type;
  Symbol* symbol;
  StructMember* member;
  Namespace* namespace_;
  Struct* parent_class;
  size_t base_index;
  SourceLocation location;
  size_t parameter_index;
  struct Attribute* annotation;
  int64_t scalar_ivalue;
  double scalar_fvalue;
  bool scalar_is_float;
  Namespace* namespace_alias_target;
  ReflectionDataMemberSpec* data_member_spec;
  ReflectionEnumeratorSpec* enumerator_spec;
  struct ASTNode* constexpr_initializer;
  Symbol* promoted_symbol;
  Vector sequence;
  Vector token_sequence;
  Symbol* substituted_template;
  Vector substituted_arguments;
  TypeRecord* extract_type;
} ReflectionValue;

ReflectionValue* ReflectionCreate(ReflectionEntityKind kind,
                                  TypeRecord* reflected_type, Symbol* symbol,
                                  StructMember* member, Namespace* namespace_,
                                  Struct* parent_class, size_t base_index,
                                  SourceLocation location);
ReflectionValue* ReflectionCreateExtended(ReflectionEntityKind kind,
                                          TypeRecord* reflected_type,
                                          Symbol* symbol, StructMember* member,
                                          Namespace* namespace_,
                                          Struct* parent_class, size_t base_index,
                                          size_t parameter_index,
                                          struct Attribute* annotation,
                                          int64_t scalar_ivalue,
                                          double scalar_fvalue,
                                          bool scalar_is_float,
                                          Namespace* namespace_alias_target,
                                          SourceLocation location);
ReflectionValue* ReflectionCreateDeserialized(ReflectionEntityKind kind,
                                              SourceLocation location);
ReflectionValue* ReflectionCreateInvalid(SourceLocation location);
ReflectionValue* ReflectionCreateType(TypeRecord* type, Symbol* alias,
                                      SourceLocation location);
ReflectionValue* ReflectionCreateSymbol(ReflectionEntityKind kind,
                                        Symbol* symbol,
                                        SourceLocation location);
ReflectionValue* ReflectionCreateMember(StructMember* member,
                                        Struct* parent_class,
                                        SourceLocation location);
ReflectionValue* ReflectionCreateNamespace(Namespace* namespace_,
                                           SourceLocation location);
ReflectionValue* ReflectionCreateBase(Struct* parent_class, size_t base_index,
                                      SourceLocation location);
ReflectionValue* ReflectionCreateParameter(Symbol* symbol, size_t index,
                                           SourceLocation location);
ReflectionValue* ReflectionCreateScalar(TypeRecord* type, int64_t ivalue,
                                        double fvalue, bool is_float,
                                        SourceLocation location);
ReflectionValue* ReflectionCreateAnnotation(struct Attribute* annotation,
                                            SourceLocation location);
ReflectionValue* ReflectionCreateNamespaceAlias(Namespace* alias_target,
                                                SourceLocation location);
ReflectionValue* ReflectionCreateDataMemberSpec(
    ReflectionDataMemberSpec* spec, SourceLocation location);
ReflectionValue* ReflectionCreateEnumeratorSpec(
    ReflectionEnumeratorSpec* spec, SourceLocation location);
ReflectionValue* ReflectionCreateSequence(ReflectionEntityKind kind,
                                          TypeRecord* element_type,
                                          Vector* values,
                                          SourceLocation location);
ReflectionValue* ReflectionCreateWithInitializer(TypeRecord* type,
                                                 struct ASTNode* initializer,
                                                 SourceLocation location);
ReflectionValue* ReflectionCreateSubstituted(Symbol* template_symbol,
                                             Vector* arguments,
                                             SourceLocation location);
ReflectionValue* ReflectionCreateTokenSequence(Vector* tokens,
                                               SourceLocation location);

TokenSequenceToken* TokenSequenceTokenNew(Token kind, const char* spelling,
                                          size_t spelling_length,
                                          SourceLocation location,
                                          struct ASTNode* pseudo_value);
TokenSequenceToken* TokenSequenceTokenCopy(const TokenSequenceToken* token);
void TokenSequenceTokenDelete(TokenSequenceToken* token);
bool TokenSequenceTokenEqual(const TokenSequenceToken* left,
                             const TokenSequenceToken* right);

ReflectionDataMemberSpec* ReflectionDataMemberSpecNew(TypeRecord* member_type);
ReflectionDataMemberSpec* ReflectionDataMemberSpecCopy(
    const ReflectionDataMemberSpec* spec);
void ReflectionDataMemberSpecDelete(ReflectionDataMemberSpec* spec);
bool ReflectionDataMemberSpecEqual(const ReflectionDataMemberSpec* left,
                                   const ReflectionDataMemberSpec* right);

ReflectionEnumeratorSpec* ReflectionEnumeratorSpecNew(void);
ReflectionEnumeratorSpec* ReflectionEnumeratorSpecCopy(
    const ReflectionEnumeratorSpec* spec);
void ReflectionEnumeratorSpecDelete(ReflectionEnumeratorSpec* spec);
bool ReflectionEnumeratorSpecEqual(const ReflectionEnumeratorSpec* left,
                                   const ReflectionEnumeratorSpec* right);

bool ReflectionValueEqual(const ReflectionValue* left,
                          const ReflectionValue* right);
ReflectionValue* ReflectionCanonicalize(ReflectionValue* value);
const char* ReflectionValueIdentifier(const ReflectionValue* value);
TypeRecord* ReflectionValueType(const ReflectionValue* value);
bool ReflectionKindIsTemplate(ReflectionEntityKind kind);
bool ReflectionKindIsNamespace(ReflectionEntityKind kind);

void ReflectionValueDelete(ReflectionValue* value);

#endif /* reflection_h */
