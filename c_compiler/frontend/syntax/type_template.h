//
//  type_template.h
//  c_compiler
//

#ifndef type_template_h
#define type_template_h

#include "type_core.h"

struct Syntax;

struct ASTNode* TypeSubstituteTemplateExpression(struct Syntax* syntax,
                                                struct ASTNode* expr,
                                                Vector* args,
                                                SourceLocation location);
struct ASTNode* TypeSubstituteTemplateExpressionAndRebase(
    struct Syntax* syntax, struct ASTNode* expr, Vector* args, int rebase_base,
    SourceLocation location);
struct ASTNode* TypeSubstituteMemberTemplateExpressionAndRebase(
    struct Syntax* syntax, struct ASTNode* expr, Vector* args, int rebase_base,
    SourceLocation location, struct Struct* from_owner,
    struct Struct* to_owner);
TypeRecord* TypeSubstituteTemplateType(struct Syntax* syntax,
                                       TypeRecord* type,
                                       Vector* args);
TypeRecord* TypeSubstituteTemplateTypeAndRebase(struct Syntax* syntax,
                                                TypeRecord* type,
                                                Vector* args,
                                                int rebase_base);
Vector* TypeSubstituteTemplateArgumentVector(struct Syntax* syntax,
                                             Vector* template_args,
                                             Vector* args);
Vector* TypeSubstituteTemplateArgumentVectorAndRebase(
    struct Syntax* syntax, Vector* template_args, Vector* args,
    int rebase_base);
// Complete a concept-id's argument list against the concept's template
// parameters, filling in trailing default arguments (which may reference the
// earlier, already-provided arguments, e.g. `C = common_type_t<T, U>`).
// Returns a freshly owned Vector of TemplateArgument* (caller frees), or NULL
// if completion is unnecessary/impossible.  Never emits diagnostics.
Vector* TypeCompleteConceptArguments(struct Syntax* syntax,
                                     Vector* concept_parameters, Vector* args);

void TypeRebaseNonDependentLambdaCallOperator(Struct* closure, Symbol* op);
Symbol* TypeInstantiateFunctionTemplate(struct Syntax* syntax, Symbol* templ,
                                        Vector* args);
void TypeEnsureTemplateMemberFunctionDefinition(struct Syntax* syntax,
                                                Symbol* symbol);
Symbol* TypeDeduceFunctionTemplateFromCall(struct Syntax* syntax, Symbol* templ,
                                           Vector* actuals);
Symbol* TypeDeduceFunctionTemplateFromCallWithExplicitArgs(
    struct Syntax* syntax, Symbol* templ, Vector* explicit_args,
    Vector* actuals);
Symbol* TypeDeduceFunctionTemplateFromCallWithOffset(struct Syntax* syntax,
                                                     Symbol* templ,
                                                     Vector* actuals,
                                                     size_t first_formal_arg);
Symbol* TypeDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
    struct Syntax* syntax, Symbol* templ, Vector* explicit_args,
    Vector* actuals, size_t first_formal_arg);
bool TypeCanDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
    Symbol* templ, Vector* explicit_args, Vector* actuals,
    size_t first_formal_arg);
typedef enum {
  kFunctionTemplateCandidateViable,
  kFunctionTemplateCandidateDeductionFailed,
  kFunctionTemplateCandidateConstraintsNotSatisfied,
} FunctionTemplateCandidateStatus;
FunctionTemplateCandidateStatus TypeClassifyFunctionTemplateCandidate(
    struct Syntax* syntax, Symbol* templ, Vector* explicit_args,
    Vector* actuals, size_t first_formal_arg);
bool TypeTemplateArgumentVectorEqual(Vector* left, Vector* right);
// Substitute explicit template arguments into a known function template's
// return type without instantiating its body.  Used to preserve the type of
// dependent calls in unevaluated contexts such as decltype.
TypeRecord* TypeSubstituteFunctionTemplateReturnType(
    struct Syntax* syntax, Symbol* function_template, Vector* explicit_args);
Symbol* TypeCreateFunctionTemplateCandidate(struct Syntax* syntax,
                                            Symbol* templ,
                                            Vector* explicit_args,
                                            Vector* actuals,
                                            size_t first_formal_arg);
// Deduce the template arguments of a conversion function template
// (`template<class T> operator T()`) for a requested target type.  A conversion
// function template has no value parameters, so its arguments are deduced by
// matching the operator's declared (dependent) target type against `target`
// ([temp.deduct.conv]).  Returns the completed template-argument vector (caller
// owns; delete with TemplateArgumentDelete), or NULL if deduction, default
// completion, or constraint checking fails.  Does not instantiate a body or
// emit diagnostics; feed the result back as explicit template arguments so the
// ordinary member-template instantiation path builds and owns the specialization.
Vector* TypeDeduceConversionOperatorTemplateArguments(struct Syntax* syntax,
                                                      Symbol* templ,
                                                      TypeRecord* target);
// Deduce a function template's arguments when its address is taken against a
// required function type ([temp.deduct.funcaddr]/[over.over]).  `target_fn` is
// the destination pointer's pointee function type.  Returns the completed
// argument vector (caller owns; delete with TemplateArgumentDelete) or NULL on
// failure.  Feed the result back as explicit template arguments to
// TypeInstantiateFunctionTemplate to build the specialization.
Vector* TypeDeduceFunctionTemplateArgumentsFromFunctionType(struct Syntax* syntax,
                                                            Symbol* templ,
                                                            Vector* explicit_args,
                                                            TypeRecord* target_fn);
// Partial ordering of two conversion function templates by their target type
// ([temp.func.order]).  Returns 1 if `a` is more specialized than `b`, -1 if
// `b` is more specialized than `a`, and 0 if neither (equivalent/incomparable,
// i.e. an ambiguous conversion).
int TypeConversionOperatorTemplateMoreSpecialized(struct Syntax* syntax,
                                                  Symbol* a, Symbol* b);
Vector* TypeDeduceFunctionTemplateArgumentsFromCall(Symbol* templ,
                                                    Vector* actuals,
                                                    size_t first_formal_arg);
TypeRecord* TypeInstantiateClassTemplate(struct Syntax* syntax, Symbol* templ,
                                         Vector* args);
TypeRecord* TypeInstantiateClassTemplateQuiet(struct Syntax* syntax,
                                              Symbol* templ, Vector* args);
// If `type` (or a pointed-to/referenced type in its spine) is a class-template
// primary carrying concrete template arguments, replace that primary with the
// corresponding specialization.  Used when a type like `variant<int,long>` is
// still represented as the primary `variant` plus args (common inside function
// templates) and member lookup must see the instantiated members.
TypeRecord* TypeMaterializeClassTemplateSpecialization(struct Syntax* syntax,
                                                       TypeRecord* type);
// Instantiate a variable template's initializer with concrete template
// arguments and constant-fold it to an integer.  Returns true on success.
bool TypeInstantiateVariableTemplateConstant(struct Syntax* syntax,
                                             Symbol* var_template, Vector* args,
                                             int64_t* out);
// Instantiate the type of a variable template (e.g. `in_place_index<1>` ->
// `in_place_index_t<1>`) with concrete template arguments.  Used for variable
// templates whose value is a class-type tag object.  Returns NULL on failure.
TypeRecord* TypeInstantiateVariableTemplateType(struct Syntax* syntax,
                                                Symbol* var_template,
                                                Vector* args);
void TypeAddCXXDeductionGuide(Symbol* class_template, Symbol* guide);
TypeRecord* TypeDeduceClassTemplateFromGuide(struct Syntax* syntax,
                                             Symbol* class_template,
                                             Vector* actuals,
                                             bool allow_explicit);
TypeRecord* TypeDeduceClassTemplateFromPlaceholder(struct Syntax* syntax,
                                                   TypeRecord* placeholder,
                                                   Vector* actuals,
                                                   bool allow_explicit,
                                                   bool* alias_rejected);
TypeRecord* TypeClassTemplatePlaceholderFromSymbol(Symbol* symbol);
bool TypeIsClassTemplatePlaceholder(TypeRecord* type);
Symbol* TypeClassTemplatePlaceholderOrigin(TypeRecord* type);
bool TypeClassTemplatePlaceholderAcceptsDeduced(TypeRecord* placeholder,
                                                TypeRecord* deduced);
bool TypeIsCXXInitializerList(TypeRecord* type);
TypeRecord* TypeCXXInitializerListElement(TypeRecord* type);
TypeRecord* TypeInstantiateCXXInitializerList(struct Syntax* syntax,
                                              TypeRecord* element_type);
TypeRecord* TypeFindCXXComparisonCategory(const char* category_name);


#endif /* type_template_h */
