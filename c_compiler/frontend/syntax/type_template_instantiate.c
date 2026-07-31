//
//  type_template_instantiate.c
//  c_compiler
//

#include "type_template_internal.h"
#include "type_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <assert.h>
#include "ast.h"
#include "compiler.h"
#include "concepts.h"
#include "constexpr.h"
#include "dstring.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "member_pointer.h"
#include "statement_semantics.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "syntax.h"
#include "semantics.h"
#include "errors.h"
#include "debug.h"
#include "rtti.h"
#include "set.h"

// When set, a parameter appearing only in a bare `T::member` non-deduced
// context is left unbound during argument deduction (so a default template
// argument can supply it) rather than deduced from the argument via this
// compiler's non-conforming member-access extension.  The extension is retried
// (flag cleared) only if a parameter is left unbound with no usable default.
static bool g_deduce_defer_bare_member = false;

static bool DeduceFunctionTemplateTypeArgument(Vector* args,
                                               size_t explicit_arg_count,
                                               TypeRecord* formal,
                                               TypeRecord* actual);
static bool DeduceFunctionTemplateStructMembers(Vector* args,
                                                size_t explicit_arg_count,
                                                TypeRecord* formal,
                                                TypeRecord* actual);
void MaxTemplateParameterIndexInArgument(TemplateArgument* arg,
                                         int* max_index);
static bool ClassTemplateArgumentPatternMatches(Vector* bindings,
                                                  TemplateArgument* pattern,
                                                  TemplateArgument* actual);
static bool ClassTemplateTypePatternMatches(Vector* bindings,
                                            TypeRecord* pattern,
                                            TypeRecord* actual);
static Vector* TemplateParameterListForSymbol(Symbol* symbol);
static bool TemplateTemplateParameterListsCompatible(Vector* formal,
                                                       Vector* actual);
static int TemplateArgumentVectorPatternSpecificity(Vector* args);
static Vector* CompleteFunctionTemplateArguments(TypeParser* parser,
                                                   TypeRecord* func,
                                                   Vector* args,
                                                   bool emit_error);
static Vector* CompleteTemplateArguments(TypeParser* parser, Vector* parameters,
                                         Vector* args, const char* error_message,
                                         bool emit_error);
static Vector* CompleteVariableTemplateArguments(TypeParser* parser,
                                                 VariableTemplate* vt,
                                                 Vector* args, bool emit_error);
static TypeRecord* InstantiateSimpleClassTemplateImpl(TypeParser* parser,
                                                      Symbol* templ,
                                                      Vector* args,
                                                      bool emit_constraint_error);
static TypeRecord* InstantiateAliasClassTemplateImpl(TypeParser* parser,
                                                   Symbol* alias, Vector* args,
                                                   bool emit_constraint_error);
static TypeRecord* InstantiateGenericAliasTemplate(TypeParser* parser,
                                                   Symbol* alias, Vector* args,
                                                   bool emit_constraint_error);
TypeRecord* TypeInstantiateClassTemplateQuiet(Syntax* syntax, Symbol* templ,
                                              Vector* args);
static bool TypeInstantiateVariableTemplateConstantImpl(
    Syntax* syntax, Symbol* var_template, Vector* args, int64_t* out,
    bool emit_constraint_error);
static ClassTemplatePartialSpecialization*
SelectVariableTemplatePartialSpecialization(TypeParser* parser, Symbol* primary,
                                            Vector* actual_args,
                                            Vector** bindings_out);
static void ReportVariableTemplateConstraintFailure(Syntax* syntax,
                                                    Symbol* var_template,
                                                    Vector* arguments);
static void ReportAliasTemplateConstraintFailure(TypeParser* parser,
                                                 Symbol* alias,
                                                 Vector* arguments);
static void ReportClassTemplateConstraintFailure(TypeParser* parser,
                                                 Symbol* templ,
                                                 ConstraintExpr* constraint,
                                                 Vector* arguments);
static TypeRecord* ClassTemplateConstraintFailureType(
    TypeParser* parser, Symbol* templ, ConstraintExpr* constraint,
    Vector* arguments, bool emit_constraint_error);

void AppendTemplateInstantiationName(String* name, Symbol* templ, Vector* args);
Vector* CompleteClassTemplateArguments(TypeParser* parser, Struct* template_struct,
                                       Vector* args);
void AddClassTemplatePartialSpecialization(TypeParser* parser, Symbol* primary,
                                           Symbol* partial_tag,
                                           Vector* pattern_args);

TypeRecord* SubstituteNestedStructTemplateParameters(TypeParser* parser,
                                                            TypeRecord* type,
                                                            Vector* args) {
  Struct* from = type->info.struct_info;
  TypeRecord* copy = TypeRecordCopy(type);
  Struct* str = NewStruct(from->is_union);
  if (parser != NULL && parser->template_substitution_target != NULL &&
      from->lexical_parent == parser->template_substitution_source) {
    str->lexical_parent = parser->template_substitution_target;
  } else if (parser != NULL &&
             parser->enclosing_template_substitution_target != NULL &&
             from->lexical_parent ==
                 parser->enclosing_template_substitution_source) {
    str->lexical_parent = parser->enclosing_template_substitution_target;
  } else {
    str->lexical_parent = from->lexical_parent;
  }
  if (StructHasMemberFunction(from)) {
    bool invented = from->tag_symbol != NULL && from->tag_symbol->flags.invented;
    String synthetic_name;
    StringInit(&synthetic_name, NULL);
    const char* tag_name = from->tag_name != NULL ? from->tag_name->value : NULL;
    if (invented || tag_name == NULL) {
      StringPrintf(&synthetic_name, "%s$S%p",
                   tag_name != NULL ? tag_name : "<anon>", (void*)str);
      tag_name = synthetic_name.value;
    }
    Symbol* tag = NewSymbol(tag_name, copy, STO(implicit));
    tag->flags.is_defined = true;
    // Preserve invented-ness so nested lambda closures rebuilt during
    // template substitution are still recognized as lambda closures (their
    // operator() bodies must be cloned, not left lazy against the template).
    if (from->tag_symbol != NULL) {
      tag->flags.invented = from->tag_symbol->flags.invented;
    }
    str->tag_name = &tag->name;
    str->tag_symbol = tag;
    StringDestruct(&synthetic_name);
  } else {
    str->tag_name = from->tag_name;
    str->tag_symbol = from->tag_symbol;
  }
  str->is_class = from->is_class;
  str->is_template = from->is_template;
  if (str->is_template && str->tag_symbol != NULL) {
    str->tag_symbol->flags.is_template = true;
  }
  VectorDestructWithContents(&str->template_parameters,
                             (VectorElementDestructor)TemplateParameterDelete,
                             /*free_element=*/false);
  VectorInit(&str->template_parameters);
  for (size_t i = 0; i < from->template_parameters.length; i++) {
    VectorAppend(&str->template_parameters,
                 TemplateParameterCopy(from->template_parameters.value.p[i]));
  }
  str->is_aggregate = from->is_aggregate;
  str->cxx_special_members_complete = from->cxx_special_members_complete;
  str->packed = from->packed;
  str->explicit_alignment = from->explicit_alignment;
  str->pack = from->pack;
  for (size_t i = 0; i < from->friend_classes.length; i++) {
    StructAddFriendClass(str, from->friend_classes.value.p[i]);
  }
  for (size_t i = 0; i < from->friend_functions.length; i++) {
    StructAddFriendFunction(str, from->friend_functions.value.p[i]);
  }
  copy->info.struct_info = str;
  copy->size = 0;

  Struct* saved_substitution_source = parser->template_substitution_source;
  Struct* saved_substitution_target = parser->template_substitution_target;
  Struct* saved_enclosing_substitution_source =
      parser->enclosing_template_substitution_source;
  Struct* saved_enclosing_substitution_target =
      parser->enclosing_template_substitution_target;
  if (saved_substitution_source != NULL && saved_substitution_target != NULL) {
    parser->enclosing_template_substitution_source =
        saved_substitution_source;
    parser->enclosing_template_substitution_target =
        saved_substitution_target;
  }
  parser->template_substitution_source = from;
  parser->template_substitution_target = str;

  for (size_t i = 0; i < from->bases.length; i++) {
    CXXBaseSpecifier* template_base = from->bases.value.p[i];
    TypeRecord* base_type =
        SubstituteTemplateParameters(parser, template_base->type, args);
    base_type = TypeMaterializeClassTemplateSpecialization(parser->syntax,
                                                           base_type);
    if (!TypeIsStructOrUnion(base_type)) {
      SyntaxError(parser->syntax, "base class must be a class or struct type");
      TypeRecordDelete(base_type);
      continue;
    }
    TypeRecordCalculateSize(base_type);
    VectorAppend(&str->bases,
                 NewCXXBaseSpecifier(base_type, template_base->access,
                                     template_base->is_virtual));
    TypeRecordDelete(base_type);
  }
  CollectCXXVirtualBases(str);
  CopyCXXBaseVirtualMembers(str);
  LayoutCXXBaseSpecifiers(str);

  Vector deferred_member_functions;
  VectorInit(&deferred_member_functions);
  for (size_t i = 0; i < from->members.length; i++) {
    StructMember* member = from->members.value.p[i];
    if (member == NULL || member->symbol == NULL) {
      continue;
    }
    if (member->symbol->flags.is_parameter_pack && !member->is_static &&
        !member->is_member_function && !member->is_using_declaration &&
        !StructMemberIsNestedType(member)) {
      int pack_index = FirstTemplateParameterIndexInType(member->symbol->type);
      TemplateArgument* pack =
          pack_index >= 0 && args != NULL && (size_t)pack_index < args->length
              ? args->value.p[pack_index]
              : NULL;
      if (pack != NULL && pack->pack_arguments != NULL) {
        for (size_t j = 0; j < pack->pack_arguments->length; j++) {
          TypeRecord* member_type = SubstituteTemplateParametersForPackElement(
              parser, member->symbol->type, args, pack_index, j);
          TypeRecordCalculateSize(member_type);
          String field_name;
          StringInit(&field_name, NULL);
          LambdaCapturePackElementName(&field_name,
                                       member->symbol->name.value, j);
          Symbol* member_symbol =
              NewSymbol(field_name.value, member_type, member->symbol->storage);
          StringDestruct(&field_name);
          member_symbol->location = member->symbol->location;
          member_symbol->flags = member->symbol->flags;
          member_symbol->flags.is_parameter_pack = false;
          member_symbol->value = member->symbol->value;
          member_symbol->dependent_value_template_parameter_index =
              member->symbol->dependent_value_template_parameter_index;
          SubstituteDependentSymbolValue(member_symbol, args);
          SubstituteStaticMemberInitializerValue(
              parser, member_symbol, member->default_initializer, args);

          StructMember* instantiated = NewStructMember(member_symbol);
          instantiated->access = member->access;
          instantiated->is_anon = member->is_anon;
          instantiated->is_static = member->is_static;
          instantiated->is_mutable = member->is_mutable;
          instantiated->is_member_function = member->is_member_function;
          instantiated->is_using_declaration = member->is_using_declaration;
          instantiated->bit_size = member->bit_size;
          instantiated->bit_offset = member->bit_offset;
          instantiated->cxx_vcall_offset = member->cxx_vcall_offset;
          AlignNextOffset(str, member_type);
          instantiated->byte_offset = str->next_offset;
          instantiated->index = str->members.length;
          AddStructMember(parser, str, instantiated);
          UpdateStructSize(str, member_type, str->is_union);
        }
        continue;
      }
    }
    if (member->is_member_function) {
      VectorAppend(&deferred_member_functions, member);
      continue;
    }
    /* A by-reference lambda capture is stored as a pointer field `T*` whose
     * pointee `T` is the captured entity's type.  When the captured variable is
     * itself a forwarding reference (`V&& var`), `T` is the enclosing parameter
     * `V`; substituting `V = U&` would form an (invalid) pointer-to-reference
     * and flag substitution failure.  For a capture field this is not a
     * SFINAE error -- capturing a reference variable by reference captures the
     * underlying object -- so collapse `(U&)*` to `U*` and keep the prior
     * substitution-failed state. */
    bool saved_capture_subst_failed = parser->template_substitution_failed;
    TypeRecord* member_type =
        SubstituteTemplateParameters(parser, member->symbol->type, args);
    if (member->symbol->flags.invented && from->tag_symbol != NULL &&
        from->tag_symbol->flags.invented && member_type != NULL &&
        TypeIsPointer(member_type) && member_type->next != NULL &&
        TypeIsReference(member_type->next)) {
      TypeRecord* reference = member_type->next;
      TypeRecord* referent = reference->next;
      TypeRecordIncRef(referent);
      member_type->next = referent;
      member_type->type = referent != NULL ? referent->type : member_type->type;
      TypeRecordDelete(reference);
      parser->template_substitution_failed = saved_capture_subst_failed;
    }
    TypeRecordCalculateSize(member_type);
    Symbol* member_symbol =
        NewSymbol(member->symbol->name.value, member_type, member->symbol->storage);
    member_symbol->location = member->symbol->location;
    member_symbol->flags = member->symbol->flags;
    member_symbol->value = member->symbol->value;
    member_symbol->dependent_value_template_parameter_index =
        member->symbol->dependent_value_template_parameter_index;
    SubstituteDependentSymbolValue(member_symbol, args);
    SubstituteStaticMemberInitializerValue(parser, member_symbol,
                                           member->default_initializer, args);

    StructMember* instantiated = NewStructMember(member_symbol);
    // Substitute template parameters in a non-static default member initializer
    // (e.g. `W value = W();`).  A plain clone would leave the parameter-typed
    // value-initialization `W()` referencing the template parameter, which then
    // lowers to an undefined symbol; substitution rewrites it to e.g. `int()`.
    if (member->default_initializer != NULL && !member->is_static) {
      ASTNode* substituted = CloneDependentExpressionWithArgs(
          parser, member->default_initializer, args);
      instantiated->default_initializer =
          substituted != NULL
              ? substituted
              : CloneCXXDefaultMemberInitializer(member->default_initializer);
    } else {
      instantiated->default_initializer =
          CloneCXXDefaultMemberInitializer(member->default_initializer);
    }
    instantiated->access = member->access;
    instantiated->is_anon = member->is_anon;
    instantiated->is_static = member->is_static;
    instantiated->is_mutable = member->is_mutable;
    instantiated->is_member_function = member->is_member_function;
    instantiated->is_using_declaration = member->is_using_declaration;
    instantiated->bit_size = member->bit_size;
    instantiated->bit_offset = member->bit_offset;
    instantiated->cxx_vcall_offset = member->cxx_vcall_offset;

    if (!instantiated->is_static && !instantiated->is_member_function &&
        !instantiated->is_using_declaration &&
        !StructMemberIsNestedType(instantiated)) {
      AlignNextOffset(str, member_type);
      instantiated->byte_offset = str->next_offset;
      instantiated->index = str->members.length;
      AddStructMember(parser, str, instantiated);
      UpdateStructSize(str, member_type, str->is_union);
    } else {
      instantiated->byte_offset = member->byte_offset;
      instantiated->index = str->members.length;
      AddStructMember(parser, str, instantiated);
    }
  }
  Vector pending_member_bodies;
  VectorInit(&pending_member_bodies);
  for (size_t i = 0; i < deferred_member_functions.length; i++) {
    StructMember* member = deferred_member_functions.value.p[i];
    StructMember* instantiated = InstantiateTemplateMemberFunction(
        parser, str, member, args, &pending_member_bodies);
    StructMember* existing =
        MapFindPointerKey(&str->symbol_table, &instantiated->symbol->name);
    if (existing != NULL) {
      AppendStructMemberOverload(parser, str, existing, instantiated);
    } else {
      AddStructMember(parser, str, instantiated);
    }
  }
  VectorDestruct(&deferred_member_functions);
  for (size_t i = 0; i < pending_member_bodies.length; i++) {
    PendingMemberBody* pmb = pending_member_bodies.value.p[i];
    CloneInstantiatedMemberFunctionBody(parser, str, pmb->symbol,
                                        pmb->template_definition,
                                        pmb->substitution_source, args);
    free(pmb);
  }
  VectorDestruct(&pending_member_bodies);
  parser->template_substitution_source = saved_substitution_source;
  parser->template_substitution_target = saved_substitution_target;
  parser->enclosing_template_substitution_source =
      saved_enclosing_substitution_source;
  parser->enclosing_template_substitution_target =
      saved_enclosing_substitution_target;
  FinalizeStructAlignment(str);
  ComputeCXXAggregateStatus(str);
  str->cxx_special_members_complete = false;
  AddImplicitCXXSpecialMembers(parser, str, str->tag_symbol);
  AddImplicitCXXDestructorIfNeeded(parser, str, str->tag_symbol);
  return TypeRecordCalculateSize(copy);
}

static void CopyFunctionTemplateParameters(TypeParser* parser, TypeRecord* to,
                                           TypeRecord* from, int rebase_base,
                                           Vector* subst_args) {
  if (to == NULL || from == NULL || !TypeIsFunction(to) ||
      !TypeIsFunction(from)) {
    return;
  }
  VectorDestructWithContents(&to->info.function.template_parameters,
                             (VectorElementDestructor)TemplateParameterDelete,
                             /*free_element=*/false);
  VectorInit(&to->info.function.template_parameters);
  for (size_t i = 0; i < from->info.function.template_parameters.length; i++) {
    TemplateParameter* original =
        from->info.function.template_parameters.value.p[i];
    if (original == NULL) {
      continue;
    }
    TemplateParameter* param =
        TemplateParameterCopy(original);
    RebaseTemplateParameterIndices(param->type, rebase_base);
    // A parameter default may name an enclosing-template parameter (indices
    // below `rebase_base`), e.g. `template <class R = D>` for a member of a
    // class template with parameter `D`.  Substitute the enclosing arguments so
    // the default becomes concrete, then rebase the member's own placeholders.
    if (param->default_type != NULL && subst_args != NULL && parser != NULL) {
      TypeRecord* substituted =
          SubstituteTemplateParameters(parser, param->default_type, subst_args);
      TypeRecordDelete(param->default_type);
      param->default_type = substituted;
    }
    RebaseTemplateParameterIndices(param->default_type, rebase_base);
    if (param->default_template_parameter_index >= rebase_base) {
      param->default_template_parameter_index -= rebase_base;
    }
    if (param->default_argument != NULL &&
        param->default_argument->template_parameter_index >= 0) {
      int default_index =
          param->default_argument->template_parameter_index;
      if (default_index < rebase_base && subst_args != NULL &&
          (size_t)default_index < subst_args->length) {
        TemplateArgument* actual = subst_args->value.p[default_index];
        if (actual != NULL) {
          TemplateArgumentDelete(param->default_argument);
          param->default_argument = TemplateArgumentCopy(actual);
        }
      } else if (default_index >= rebase_base) {
        param->default_argument->template_parameter_index -= rebase_base;
      }
    }
    if (param->index >= rebase_base) {
      param->index -= rebase_base;
    }
    VectorAppend(&to->info.function.template_parameters, param);
  }
}

void AppendTemplateInstantiationName(String* name, Symbol* templ,
                                            Vector* args) {
  Struct* template_struct =
      templ->type != NULL && TypeIsStructOrUnion(templ->type)
          ? templ->type->info.struct_info
          : NULL;
  if (template_struct != NULL && template_struct->lexical_parent != NULL) {
    StringPrintf(name, "$nested%d$", templ->id);
    StringAppendString(name, &templ->name);
  } else {
    StringSet(name, templ->name.value);
  }
  StringAppendChar(name, '<');
  for (size_t i = 0; i < args->length; i++) {
    if (i != 0) {
      StringAppend(name, ",");
    }
    String arg_name;
    StringInit(&arg_name, NULL);
    TemplateArgument* arg = args->value.p[i];
    if (arg->pack_arguments != NULL) {
      StringAppendChar(&arg_name, '[');
      for (size_t j = 0; j < arg->pack_arguments->length; j++) {
        if (j != 0) {
          StringAppend(&arg_name, ",");
        }
        String element_name;
        StringInit(&element_name, NULL);
        TemplateArgument* element = arg->pack_arguments->value.p[j];
        if (element->kind == kTemplateParameterType) {
          TypeRecordToTemplateKeyString(element->type, &element_name);
        } else if (element->kind == kTemplateParameterTemplate) {
          if (element->template_parameter_index >= 0) {
            StringPrintf(&element_name, "$TT%d",
                         element->template_parameter_index);
          } else {
            StringPrintf(&element_name, "TT%d",
                         element->template_symbol != NULL
                             ? element->template_symbol->id : -1);
          }
        } else if (element->template_parameter_index >= 0) {
          StringPrintf(&element_name, "$N%d", element->template_parameter_index);
        } else {
          char buffer[32];
          snprintf(buffer, sizeof(buffer), "%lld", element->int_value);
          StringAppend(&element_name, buffer);
        }
        StringAppendString(&arg_name, &element_name);
        StringDestruct(&element_name);
      }
      StringAppendChar(&arg_name, ']');
    } else if (arg->kind == kTemplateParameterType) {
      TypeRecordToTemplateKeyString(arg->type, &arg_name);
    } else if (arg->kind == kTemplateParameterTemplate) {
      if (arg->template_parameter_index >= 0) {
        StringPrintf(&arg_name, "$TT%d", arg->template_parameter_index);
      } else {
        StringPrintf(&arg_name, "TT%d",
                     arg->template_symbol != NULL
                         ? arg->template_symbol->id : -1);
      }
    } else {
      if (arg->template_parameter_index >= 0) {
        StringPrintf(&arg_name, "$N%d", arg->template_parameter_index);
      } else {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%lld", arg->int_value);
        StringAppend(&arg_name, buffer);
      }
    }
    StringAppendString(name, &arg_name);
    StringDestruct(&arg_name);
  }
  StringAppendChar(name, '>');
}

/* Find an already-created instantiation tag named `name` for template `templ`,
 * or NULL. */
static Symbol* FindTemplateInstantiationTag(TypeParser* parser, Symbol* templ,
                                            String* name) {
  (void)parser;
  /* Look up the instantiation tag in exactly the template's own namespace.
   * Using a scope-walking lookup (SyntaxFindTag) here is wrong: two templates
   * with the same name in different namespaces (e.g. ::Foo<int> and
   * ns::Foo<int>) produce identical instantiation-name strings, so falling back
   * to enclosing/global scopes would alias the nested instantiation to a
   * previously created global one, corrupting its template_origin. This mirrors
   * how AddTemplateInstantiationTag inserts the tag. */
  if (templ->namespace_ != NULL &&
      templ->namespace_ != compiler->global_namespace) {
    return NamespaceFindTag(templ->namespace_, name);
  }
  return FindGlobalTag(name);
}

/* Register a freshly created instantiation tag in the template's own namespace
 * (temporarily switching the parser's tag scope so the tag is not added to some
 * unrelated local/current scope). Mirrors FindTemplateInstantiationTag. */
static bool AddTemplateInstantiationTag(TypeParser* parser, Symbol* templ,
                                        Symbol* tag) {
  LocalSymbolTable* saved_tag_stack = parser->syntax->local_tag_stack;
  Namespace* saved_namespace = parser->syntax->current_namespace;
  parser->syntax->local_tag_stack = NULL;
  parser->syntax->current_namespace =
      templ->namespace_ != NULL ? templ->namespace_ : compiler->global_namespace;
  bool added = SyntaxAddTag(parser->syntax, tag);
  parser->syntax->local_tag_stack = saved_tag_stack;
  parser->syntax->current_namespace = saved_namespace;
  return added;
}

/* Reject (with a diagnostic) class-template instantiations that use member
 * kinds the instantiation machinery does not yet handle: anonymous members and
 * bit-fields.  Virtual (and pure-virtual) member functions *are* supported: the
 * instantiation path completes the polymorphic layout and emits the vtable(s)
 * the same way a normal class definition does. */
static bool ClassTemplateInstantiationMembersSupported(TypeParser* parser,
                                                       Struct* str) {
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (StructMemberIsNestedType(member)) {
      continue;
    }
    if (member->is_anon || StructMemberIsBitField(member)) {
      SyntaxError(parser->syntax,
                  "Class template instantiation is not supported yet");
      return false;
    }
    if (member->is_member_function &&
        (member->symbol == NULL || member->symbol->type == NULL ||
         !TypeIsFunction(member->symbol->type))) {
      SyntaxError(parser->syntax,
                  "Class template instantiation is not supported yet");
      return false;
    }
  }
  return true;
}

/* Build the concrete function type for a member function of an instantiated
 * class template: copy all of `from`'s function attributes (cv/ref/noexcept,
 * special-member kind, coroutine info, etc.), substitute the return type and
 * each parameter type against `args` (expanding parameter packs), re-add the
 * implicit `this` parameter against the instantiated `owner`, and renumber
 * argument slots. */
static TypeRecord* InstantiateMemberFunctionType(TypeParser* parser,
                                                 Struct* owner,
                                                 bool is_static_member,
                                                 TypeRecord* from,
                                                 Vector* args,
                                                 SourceLocation location) {
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.varargs = from->info.function.varargs;
  func->info.function.unknown_args = from->info.function.unknown_args;
  func->info.function.definition = false;
  func->info.function.is_inline = from->info.function.is_inline;
  func->info.function.is_constexpr = from->info.function.is_constexpr;
  func->info.function.is_consteval = from->info.function.is_consteval;
  func->info.function.is_constructor = from->info.function.is_constructor;
  func->info.function.is_destructor = from->info.function.is_destructor;
  func->info.function.is_const_member = from->info.function.is_const_member;
  func->info.function.is_volatile_member =
      from->info.function.is_volatile_member;
  func->info.function.has_explicit_object_parameter =
      from->info.function.has_explicit_object_parameter;
  func->info.function.ref_qualifier = from->info.function.ref_qualifier;
  func->info.function.is_explicit = from->info.function.is_explicit;
  func->info.function.is_explicit_conversion =
      from->info.function.is_explicit_conversion;
  func->info.function.explicit_condition = NULL;
  func->info.function.is_final = from->info.function.is_final;
  // Virtualness must survive instantiation so the instantiated member is
  // registered into a vtable slot (RegisterCXXVirtualMember, run via
  // AddStructMember, keys off is_virtual).  The concrete slot index is *not*
  // copied: it is reassigned during registration by override resolution against
  // the instantiated bases, matching how a normal class assigns slots.
  func->info.function.is_virtual = from->info.function.is_virtual;
  func->info.function.is_override = from->info.function.is_override;
  func->info.function.is_pure_virtual = from->info.function.is_pure_virtual;
  func->info.function.is_defaulted = from->info.function.is_defaulted;
  func->info.function.is_deleted = from->info.function.is_deleted;
  func->info.function.cxx_special_member_kind =
      from->info.function.cxx_special_member_kind;
  func->info.function.is_user_declared = from->info.function.is_user_declared;
  func->info.function.is_user_provided = from->info.function.is_user_provided;
  func->info.function.is_explicitly_defaulted =
      from->info.function.is_explicitly_defaulted;
  func->info.function.is_explicitly_deleted =
      from->info.function.is_explicitly_deleted;
  func->info.function.is_implicitly_declared =
      from->info.function.is_implicitly_declared;
  func->info.function.is_implicitly_deleted =
      from->info.function.is_implicitly_deleted;
  func->info.function.is_trivial_special_member =
      from->info.function.is_trivial_special_member;
  func->info.function.is_constexpr_eligible =
      from->info.function.is_constexpr_eligible;
  func->info.function.is_noexcept_eligible =
      from->info.function.is_noexcept_eligible;
  func->info.function.is_noexcept = from->info.function.is_noexcept;
  func->info.function.is_auto_return_deduced =
      from->info.function.is_auto_return_deduced;
  func->info.function.is_decltype_auto_return_deduced =
      from->info.function.is_decltype_auto_return_deduced;
  func->info.function.is_coroutine = from->info.function.is_coroutine;
  func->info.function.coroutine_promise_type =
      from->info.function.coroutine_promise_type != NULL
          ? TypeRecordCopy(from->info.function.coroutine_promise_type)
          : NULL;
  func->info.function.coroutine_frame_type =
      from->info.function.coroutine_frame_type != NULL
          ? TypeRecordCopy(from->info.function.coroutine_frame_type)
          : NULL;
  func->info.function.coroutine_suspend_count =
      from->info.function.coroutine_suspend_count;
  func->info.function.template_parameter_count =
      from->info.function.template_parameter_count;
  func->info.function.template_parameter_base = 0;
  int member_template_base = from->info.function.template_parameter_base;
  // A member function template's own parameters are numbered at/after
  // `member_template_base`.  Only enclosing-template arguments (indices
  // below that base) may be substituted here; the member's own placeholders
  // must survive for later deduction.  When the member has already been
  // rebased to a standalone template (base == 0) and is rebuilt again —
  // e.g. a nested generic lambda whose closure is substituted while cloning
  // an outer generic-lambda body — the enclosing args would otherwise
  // consume the member's own `auto` placeholders at index 0.
  Vector enclosing_only_args;
  Vector* subst_args = args;
  bool use_enclosing_only = false;
  if (from->info.function.template_parameter_count > 0) {
    VectorInit(&enclosing_only_args);
    use_enclosing_only = true;
    for (size_t i = 0;
         args != NULL && (int)i < member_template_base && i < args->length;
         i++) {
      VectorAppend(&enclosing_only_args, args->value.p[i]);
    }
    subst_args = &enclosing_only_args;
  }
  // A member function template's explicit condition may depend on both its
  // enclosing class and its own template parameters.  Substitute only the
  // enclosing arguments here, then rebase the surviving member parameters to
  // the standalone function template's zero-based numbering.  Using `args`
  // directly would consume member parameters that happen to share an index
  // with an enclosing parameter and incorrectly make explicit constructors
  // implicit.
  if (from->info.function.explicit_condition != NULL) {
    ASTNode* condition = TypeSubstituteMemberTemplateExpressionAndRebase(
        parser->syntax, from->info.function.explicit_condition, subst_args,
        member_template_base, from->info.function.explicit_condition->location,
        parser->template_substitution_source,
        parser->template_substitution_target);
    if (ExpressionIsTemplateDependent(condition)) {
      func->info.function.explicit_condition = condition;
    } else {
      int64_t explicit_value = 0;
      DiagnosticSuppressBegin();
      condition = AnalyzeExpression(condition);
      bool folded = EvaluateIntegerExpression(condition, &explicit_value);
      DiagnosticSuppressEnd();
      if (folded) {
        func->info.function.is_explicit = explicit_value != 0;
        func->info.function.is_explicit_conversion =
            from->info.function.is_explicit_conversion && explicit_value != 0;
      }
      ASTNodeDelete(condition);
    }
  }
  // Copy the member's own template parameters, substituting enclosing arguments
  // into each parameter's default (e.g. a `= D` default that names the enclosing
  // class parameter) before rebasing.  Without this substitution the default is
  // left dangling and a parameter that can only be supplied by its default
  // (appearing solely in a non-deduced context) fails deduction.
  CopyFunctionTemplateParameters(parser, func, from, member_template_base,
                                 subst_args);
  TypeRecord* return_type =
      SubstituteTemplateParameters(parser, from->next, subst_args);
  RebaseTemplateParameterIndices(return_type, member_template_base);
  TypeRecordChain(func, return_type);

  if (is_static_member ||
      from->info.function.has_explicit_object_parameter) {
    func->info.function.cxx_member_owner = owner;
  } else {
    TypeRecordAddCXXThisParameter(func, owner, location);
  }
  size_t first_formal =
      from->info.function.cxx_member_owner != NULL && !is_static_member &&
              !from->info.function.has_explicit_object_parameter
          ? 1
          : 0;
  for (size_t i = first_formal; i < from->info.function.prototype.length; i++) {
    Symbol* formal = from->info.function.prototype.value.p[i];
    // The implicit `__complete_object` flag (present on constructors/destructors
    // of a class with virtual bases) is recreated by TypeRecordAddCXXThisParameter
    // for the instantiated function above; copying the source's copy as well would
    // duplicate it, giving the instantiated constructor a spurious extra int
    // parameter and breaking every call to it.
    if (formal != NULL && StringEqual(&formal->name, "__complete_object")) {
      continue;
    }
    if (formal != NULL && formal->flags.is_parameter_pack) {
      AppendSubstitutedFormalParameter(
          parser, &func->info.function.prototype, formal, subst_args,
          member_template_base);
      continue;
    }
    TypeRecord* formal_type =
        SubstituteTemplateParameters(parser, formal->type, subst_args);
    RebaseTemplateParameterIndices(formal_type, member_template_base);
    Symbol* clone = NewSymbol(formal->name.value, formal_type, formal->storage);
    clone->flags = formal->flags;
    clone->flags.is_argument = true;
    clone->location = formal->location;
    clone->default_argument =
        ASTNodeClone(formal->default_argument, IdentityCloneNode, NULL, NULL);
    VectorAppend(&func->info.function.prototype, clone);
  }
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    formal->value.arg_number = (int32_t)i;
  }
  if (from->info.function.associated_constraint != NULL) {
    // Substituting an associated constraint is a purely type-level operation:
    // it resolves alias/decltype types such as `iterator_t<D>` (which itself is
    // `decltype(ranges::begin(declval<D&>()))`).  For a CRTP base whose Derived
    // is still incomplete at this point, such resolution can transiently name
    // helper function templates (e.g. `std::forward`/`std::move`) with degenerate
    // arguments.  Those must never be cloned, queued, or codegen'd here, so
    // perform the substitution in speculative (signature-only) mode.
    //
    // Resolving those alias/decltype types can also transiently *fail* while
    // Derived is incomplete (e.g. `ranges::begin(D&)` where `D::begin` is not
    // yet visible), emitting diagnostics.  This is only a best-effort signature
    // substitution: the constraint is re-checked properly when the member is
    // actually named/called.  Trap any such errors so they cannot leak into --
    // and poison -- an enclosing speculative context (e.g. a requires-expression
    // that merely instantiates this class), which would otherwise report the
    // requirement as unsatisfied.
    bool saved_trap = DiagnosticErrorTrapBegin();
    compiler->speculative_template_instantiation_depth++;
    func->info.function.associated_constraint =
        ConceptsSubstituteMemberConstraint(
            parser->syntax, from->info.function.associated_constraint,
            subst_args, member_template_base,
            from->info.function.cxx_member_owner, owner);
    compiler->speculative_template_instantiation_depth--;
    DiagnosticErrorTrapEnd(saved_trap);
  }
  if (use_enclosing_only) {
    // Shallow: elements are borrowed from `args`, not owned here.
    VectorDestruct(&enclosing_only_args);
  }
  (void)parser;
  return func;
}

bool TypeInstantiateVariableTemplateConstant(Syntax* syntax,
                                             Symbol* var_template, Vector* args,
                                             int64_t* out) {
  return TypeInstantiateVariableTemplateConstantImpl(syntax, var_template, args,
                                                       out,
                                                       /*emit_constraint_error=*/true);
}

static bool TypeInstantiateVariableTemplateConstantImpl(
    Syntax* syntax, Symbol* var_template, Vector* args, int64_t* out,
    bool emit_constraint_error) {
  if (var_template == NULL || var_template->variable_template == NULL ||
      var_template->variable_template->initializer == NULL) {
    return false;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  Vector* completed_args = CompleteVariableTemplateArguments(
      &parser, var_template->variable_template, args, emit_constraint_error);
  if (completed_args == NULL) {
    TypeParserDestruct(&parser);
    return false;
  }
  // A matching partial (or explicit) specialization supplies its own
  // initializer and constraint, folded with the arguments deduced from the
  // specialization's pattern (its "bindings").
  Vector* partial_args = NULL;
  ClassTemplatePartialSpecialization* partial =
      SelectVariableTemplatePartialSpecialization(&parser, var_template,
                                                  completed_args, &partial_args);
  ASTNode* initializer = var_template->variable_template->initializer;
  ConstraintExpr* constraint =
      var_template->variable_template->associated_constraint;
  Vector* fold_args = completed_args;
  if (partial != NULL) {
    initializer = partial->variable_initializer;
    constraint = partial->associated_constraint;
    fold_args = partial_args;
  }
  bool ok = false;
  if (!ConceptsConstraintSatisfied(constraint, fold_args)) {
    if (emit_constraint_error && partial == NULL) {
      ReportVariableTemplateConstraintFailure(syntax, var_template,
                                              completed_args);
    }
  } else if (initializer != NULL) {
    ok = TryFoldDependentTemplateArgument(&parser, initializer, fold_args, out);
  }
  if (partial_args != NULL) {
    VectorDeleteWithContents(partial_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  TypeParserDestruct(&parser);
  return ok;
}

/* Instantiate the *type* of a C++ variable template against concrete template
 * arguments `args`, e.g. `in_place_index<1>` -> `in_place_index_t<1>`.  Used for
 * variable templates whose value is a class-type object (a tag such as
 * `std::in_place_index`) rather than a folded constant.  Returns a freshly
 * allocated concrete type, or NULL if the template has no type. */
TypeRecord* TypeInstantiateVariableTemplateType(Syntax* syntax,
                                                Symbol* var_template,
                                                Vector* args) {
  if (var_template == NULL || var_template->variable_template == NULL ||
      var_template->type == NULL) {
    return NULL;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  Vector* completed_args = CompleteVariableTemplateArguments(
      &parser, var_template->variable_template, args, /*emit_error=*/true);
  if (completed_args == NULL) {
    TypeParserDestruct(&parser);
    return NULL;
  }
  if (!ConceptsConstraintSatisfied(
          var_template->variable_template->associated_constraint,
          completed_args)) {
    ReportVariableTemplateConstraintFailure(syntax, var_template, completed_args);
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    TypeParserDestruct(&parser);
    return NULL;
  }
  TypeRecord* concrete =
      SubstituteTemplateParameters(&parser, var_template->type, completed_args);
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  TypeParserDestruct(&parser);
  return concrete;
}

static TypeRecord* InstantiateFunctionTemplateType(TypeParser* parser,
                                                   TypeRecord* from,
                                                   Vector* args) {
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.varargs = from->info.function.varargs;
  func->info.function.unknown_args = from->info.function.unknown_args;
  func->info.function.definition = false;
  func->info.function.old_style = from->info.function.old_style;
  func->info.function.is_inline = from->info.function.is_inline;
  func->info.function.is_constexpr = from->info.function.is_constexpr;
  func->info.function.is_consteval = from->info.function.is_consteval;
  func->info.function.is_const_member = from->info.function.is_const_member;
  func->info.function.is_volatile_member =
      from->info.function.is_volatile_member;
  func->info.function.has_explicit_object_parameter =
      from->info.function.has_explicit_object_parameter;
  func->info.function.ref_qualifier = from->info.function.ref_qualifier;
  // Preserve constructor/destructor-ness so an instantiated constructor
  // template is still recognized as a constructor (its call is void-typed and
  // must not be treated as a copy-initialization of the object).
  func->info.function.is_constructor = from->info.function.is_constructor;
  func->info.function.is_destructor = from->info.function.is_destructor;
  func->info.function.is_explicit = from->info.function.is_explicit;
  func->info.function.is_explicit_conversion =
      from->info.function.is_explicit_conversion;
  func->info.function.explicit_condition = NULL;
  if (from->info.function.explicit_condition != NULL) {
    int64_t explicit_value = 0;
    if (TryFoldDependentTemplateArgument(
            parser, from->info.function.explicit_condition, args,
            &explicit_value)) {
      func->info.function.is_explicit = explicit_value != 0;
      func->info.function.is_explicit_conversion =
          from->info.function.is_explicit_conversion && explicit_value != 0;
    } else {
      ASTNode* condition = CloneDependentExpressionWithArgs(
          parser, from->info.function.explicit_condition, args);
      if (DependentExpressionContainsTemplateParameter(condition)) {
        func->info.function.explicit_condition = condition;
      } else {
        ASTNodeDelete(condition);
      }
    }
  }
  func->info.function.is_virtual = from->info.function.is_virtual;
  func->info.function.is_override = from->info.function.is_override;
  func->info.function.is_final = from->info.function.is_final;
  func->info.function.is_pure_virtual = from->info.function.is_pure_virtual;
  func->info.function.is_defaulted = from->info.function.is_defaulted;
  func->info.function.is_deleted = from->info.function.is_deleted;
  func->info.function.cxx_special_member_kind =
      from->info.function.cxx_special_member_kind;
  func->info.function.is_user_declared = from->info.function.is_user_declared;
  func->info.function.is_user_provided = from->info.function.is_user_provided;
  func->info.function.is_explicitly_defaulted =
      from->info.function.is_explicitly_defaulted;
  func->info.function.is_explicitly_deleted =
      from->info.function.is_explicitly_deleted;
  func->info.function.is_implicitly_declared =
      from->info.function.is_implicitly_declared;
  func->info.function.is_implicitly_deleted =
      from->info.function.is_implicitly_deleted;
  func->info.function.is_trivial_special_member =
      from->info.function.is_trivial_special_member;
  func->info.function.is_constexpr_eligible =
      from->info.function.is_constexpr_eligible;
  func->info.function.is_noexcept_eligible =
      from->info.function.is_noexcept_eligible;
  func->info.function.is_noexcept = from->info.function.is_noexcept;
  func->info.function.is_auto_return_deduced =
      from->info.function.is_auto_return_deduced;
  func->info.function.is_decltype_auto_return_deduced =
      from->info.function.is_decltype_auto_return_deduced;
  func->info.function.is_deduction_guide =
      from->info.function.is_deduction_guide;
  func->info.function.is_coroutine = from->info.function.is_coroutine;
  func->info.function.coroutine_promise_type =
      from->info.function.coroutine_promise_type != NULL
          ? TypeRecordCopy(from->info.function.coroutine_promise_type)
          : NULL;
  func->info.function.coroutine_frame_type =
      from->info.function.coroutine_frame_type != NULL
          ? TypeRecordCopy(from->info.function.coroutine_frame_type)
          : NULL;
  func->info.function.coroutine_suspend_count =
      from->info.function.coroutine_suspend_count;
  func->info.function.virtual_index = from->info.function.virtual_index;
  func->info.function.cxx_member_owner = from->info.function.cxx_member_owner;
  TypeRecord* return_type = SubstituteTemplateParameters(parser, from->next, args);
  TypeRecordChain(func, return_type);

  for (size_t i = 0; i < from->info.function.prototype.length; i++) {
    Symbol* formal = from->info.function.prototype.value.p[i];
    if (formal != NULL && formal->flags.is_parameter_pack) {
      AppendSubstitutedFormalParameter(parser, &func->info.function.prototype,
                                       formal, args, 0);
      continue;
    }
    TypeRecord* formal_type =
        SubstituteTemplateParameters(parser, formal->type, args);
    Symbol* clone = NewSymbol(formal->name.value, formal_type, formal->storage);
    clone->flags = formal->flags;
    clone->flags.is_argument = true;
    clone->location = formal->location;
    clone->default_argument =
        ASTNodeClone(formal->default_argument, IdentityCloneNode, NULL, NULL);
    VectorAppend(&func->info.function.prototype, clone);
  }
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    formal->value.arg_number = (int32_t)i;
  }
  return func;
}

/* Structural equality of two concrete template arguments (kind, pack contents,
 * type, or non-type value). Used to find an existing matching instantiation. */
bool TypeTemplateArgumentVectorEqual(Vector* left, Vector* right) {
  if (TemplateArgumentVectorEqual(left, right)) {
    return true;
  }
  if (left == NULL || right == NULL || left->length != right->length) {
    return false;
  }
  for (size_t i = 0; i < left->length; i++) {
    TemplateArgument* left_arg = left->value.p[i];
    TemplateArgument* right_arg = right->value.p[i];
    if (TemplateArgumentEqual(left_arg, right_arg)) {
      continue;
    }
    if (left_arg != NULL && left_arg->pack_arguments != NULL &&
        left_arg->pack_arguments->length == 1 &&
        TemplateArgumentEqual(left_arg->pack_arguments->value.p[0],
                              right_arg)) {
      continue;
    }
    if (right_arg != NULL && right_arg->pack_arguments != NULL &&
        right_arg->pack_arguments->length == 1 &&
        TemplateArgumentEqual(left_arg,
                              right_arg->pack_arguments->value.p[0])) {
      continue;
    }
    return false;
  }
  return true;
}

/* Structural equality of two value-dependent non-type template-argument
 * expressions (e.g. the conditions of two `enable_if` SFINAE overloads).  Only
 * the node shapes that appear in constant/SFINAE conditions are compared; any
 * other shape is conservatively treated as unequal so that distinct overloads
 * are never merged into one. */
/* Stack of function-template specializations whose bodies are currently being
 * cloned.  A self-recursive function template (e.g. an introsort or merge-sort
 * helper that calls itself on subranges) contains a call to the very
 * specialization being instantiated.  Cloning its body triggers that call,
 * which re-enters instantiation and finds the in-progress specialization.  The
 * body has not been assigned yet at that point, so without this marker the
 * re-entry would clone the body again, recursing until the stack overflows.
 * Instantiation runs single-threaded, so a plain intrusive stack suffices. */
typedef struct FunctionInstantiationInProgress {
  Symbol* symbol;
  struct FunctionInstantiationInProgress* next;
} FunctionInstantiationInProgress;
static FunctionInstantiationInProgress* g_function_instantiations_in_progress =
    NULL;

static bool FunctionTemplateInstantiationInProgress(Symbol* symbol) {
  for (FunctionInstantiationInProgress* node =
           g_function_instantiations_in_progress;
       node != NULL; node = node->next) {
    if (node->symbol == symbol) {
      return true;
    }
  }
  return false;
}

static void PushFunctionInstantiationInProgress(
    FunctionInstantiationInProgress* node, Symbol* symbol) {
  node->symbol = symbol;
  node->next = g_function_instantiations_in_progress;
  g_function_instantiations_in_progress = node;
}

static void PopFunctionInstantiationInProgress(
    FunctionInstantiationInProgress* node) {
  g_function_instantiations_in_progress = node->next;
}

static Vector* FunctionTemplateBodyArguments(Symbol* template_definition,
                                             Symbol* symbol,
                                             Vector* member_args) {
  if (template_definition == NULL || template_definition->type == NULL ||
      symbol == NULL || symbol->type == NULL ||
      !TypeIsFunction(template_definition->type) ||
      !TypeIsFunction(symbol->type)) {
    return member_args;
  }
  int enclosing_count =
      template_definition->type->info.function.template_parameter_base;
  for (size_t i = 0;
       enclosing_count > 0 &&
       i < template_definition->type->info.function.prototype.length;
       i++) {
    Symbol* formal =
        template_definition->type->info.function.prototype.value.p[i];
    int parameter_index =
        formal != NULL ? FirstTemplateParameterIndexInType(formal->type) : -1;
    if (parameter_index >= 0 && parameter_index < enclosing_count) {
      // This body belongs to the member as cloned into an already-instantiated
      // class.  Its own parameters have been rebased to zero, so its argument
      // vector must stay member-local.  Argument completion may nevertheless
      // have retained the enclosing arguments in front of the member arguments;
      // strip that prefix or parameter zero binds to the class argument instead
      // of the member specialization's first argument.
      size_t own_count =
          template_definition->type->info.function.template_parameters.length;
      if (member_args == NULL || own_count == 0 ||
          member_args->length <= own_count) {
        return member_args;
      }
      Vector* local_args = NewVector();
      size_t first = member_args->length - own_count;
      for (size_t j = first; j < member_args->length; j++) {
        VectorAppend(local_args,
                     TemplateArgumentCopy(member_args->value.p[j]));
      }
      return local_args;
    }
  }
  Symbol* member_template = symbol->type->info.function.template_origin;
  Vector* enclosing_args =
      member_template != NULL && member_template->type != NULL
          ? member_template->type->template_arguments
          : NULL;
  if (enclosing_count <= 0 || enclosing_args == NULL ||
      enclosing_args->length < (size_t)enclosing_count) {
    return member_args;
  }
  Vector* combined = NewVector();
  for (int i = 0; i < enclosing_count; i++) {
    VectorAppend(combined,
                 TemplateArgumentCopy(enclosing_args->value.p[i]));
  }
  for (size_t i = 0; member_args != NULL && i < member_args->length; i++) {
    VectorAppend(combined, TemplateArgumentCopy(member_args->value.p[i]));
  }
  return combined;
}

static void EnsureFunctionTemplateInstantiationQueued(TypeParser* parser,
                                                      Symbol* template_definition,
                                                      Symbol* symbol,
                                                      Vector* args) {
  if (parser == NULL || template_definition == NULL ||
      template_definition->type == NULL || symbol == NULL ||
      symbol->type == NULL ||
      template_definition->type->info.function.body == NULL ||
      symbol->type->info.function.body != NULL ||
      FunctionTemplateInstantiationInProgress(symbol) ||
      PendingTemplateInstantiationHasAsmName(symbol->asm_name.value)) {
    return;
  }
  FunctionInstantiationInProgress in_progress;
  PushFunctionInstantiationInProgress(&in_progress, symbol);
  Vector* body_args =
      FunctionTemplateBodyArguments(template_definition, symbol, args);
  symbol->type->info.function.body =
      CloneTemplateFunctionBody(parser, template_definition->type,
                                symbol->type, body_args);
  PopFunctionInstantiationInProgress(&in_progress);
  if (body_args != args) {
    VectorDeleteWithContents(body_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  SyntaxInsertClonedTemplateConstructorPreamble(parser, template_definition,
                                                symbol, args);
  symbol->type->info.function.definition = true;
  symbol->flags.is_defined = true;
  if (symbol->type->info.function.is_inline) {
    symbol->flags.is_inline_defn = true;
    if (!StorageIs(symbol->storage, STO(static))) {
      symbol->flags.is_weak = true;
    }
  }
  Vector* declarations = NewVector();
  VectorAppend(declarations,
               NewVariableDeclarationASTNode(symbol, NULL, symbol->location));
  VectorAppend(&compiler->pending_template_instantiations,
               NewDeclarationListASTNode(declarations, symbol->location));
  VectorAppend(&compiler->declaration_asts, symbol->type->info.function.body);
}

static bool TemplateArgumentContainsTemplateParameterForInstantiation(
    TemplateArgument* arg) {
  if (arg == NULL) {
    return false;
  }
  if (arg->template_parameter_index >= 0 ||
      TypeContainsTemplateParameter(arg->type) ||
      DependentExpressionContainsTemplateParameter(arg->dependent_expr)) {
    return true;
  }
  for (size_t i = 0; arg->pack_arguments != NULL &&
                     i < arg->pack_arguments->length; i++) {
    if (TemplateArgumentContainsTemplateParameterForInstantiation(
            arg->pack_arguments->value.p[i])) {
      return true;
    }
  }
  return false;
}

/* True if any argument in the vector is still dependent (see above). */
static bool TemplateArgumentVectorContainsTemplateParameterForInstantiation(
    Vector* args) {
  for (size_t i = 0; args != NULL && i < args->length; i++) {
    if (TemplateArgumentContainsTemplateParameterForInstantiation(
            args->value.p[i])) {
      return true;
    }
  }
  return false;
}

/* Re-analyze one assignment whose operand conversion was deferred because an
 * operand was still type-dependent when the enclosing member function template
 * was first (class-level) instantiated.  Now that the body has been cloned with
 * concrete arguments the correct conversion can be selected. */
static Symbol* InstantiateSimpleFunctionTemplate(TypeParser* parser,
                                                 Symbol* templ,
                                                 Vector* args) {
  if (templ == NULL || templ->type == NULL || !templ->flags.is_template ||
      !TypeIsFunction(templ->type)) {
    return templ;
  }
  if (templ->type->info.function.template_origin != NULL &&
      templ->type->template_arguments != NULL &&
      templ->type->info.function.body != NULL &&
      !TypeContainsTemplateParameter(templ->type)) {
    // This is already a concrete specialization.  Re-instantiating it with its
    // retained arguments substitutes pointer/reference declarators a second
    // time (for example allocator<T>::destroy<U>(U*) becoming U**).
    return templ;
  }
  if (templ->type->info.function.template_origin != NULL &&
      templ->type->template_arguments == NULL &&
      (!templ->is_imported_module_symbol ||
       templ->type->info.function.template_parameters.length == 0)) {
    templ = templ->type->info.function.template_origin;
  }
  TypeRecord* completion_type = templ->type;
  if (templ->is_imported_module_symbol && completion_type != NULL &&
      TypeIsFunction(completion_type) &&
      completion_type->info.function.template_parameters.length == 0 &&
      templ->value.func_defn != NULL && templ->value.func_defn->type != NULL &&
      TypeIsFunction(templ->value.func_defn->type) &&
      templ->value.func_defn->type->info.function.template_parameters.length > 0) {
    completion_type = templ->value.func_defn->type;
  }
  Vector* completed_args =
      CompleteFunctionTemplateArguments(parser, completion_type, args,
                                        /*emit_error=*/true);
  if (completed_args == NULL) {
    return templ;
  }
  if (TemplateArgumentVectorContainsTemplateParameterForInstantiation(
          completed_args)) {
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return templ;
  }
  Symbol* template_definition = templ;
  if ((completion_type != templ->type ||
       template_definition->type == NULL ||
       template_definition->type->info.function.body == NULL) &&
      templ->value.func_defn != NULL &&
      templ->value.func_defn->type != NULL &&
      templ->value.func_defn->type->info.function.body != NULL) {
    template_definition = templ->value.func_defn;
  }
  if (template_definition->type->info.function.body == NULL) {
    SyntaxError(parser->syntax,
                "Function template definition is required for instantiation");
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return templ;
  }
  bool saved_substitution_failed = parser->template_substitution_failed;
  parser->template_substitution_failed = false;
  TypeRecord* func =
      InstantiateFunctionTemplateType(parser, completion_type,
                                      completed_args);
  bool substitution_failed = parser->template_substitution_failed;
  parser->template_substitution_failed = saved_substitution_failed;
  if (substitution_failed) {
    // A dependent type in the signature (e.g. an `enable_if` SFINAE guard) had
    // no valid substitution.  Abandon this instantiation quietly so overload
    // resolution can discard the candidate; do not create or queue a symbol.
    TypeRecordDelete(func);
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return templ;
  }
  Symbol* existing = FindFunctionTemplateInstantiation(templ, func,
                                                       completed_args);
  if (existing != NULL) {
    if (compiler->speculative_template_instantiation_depth == 0) {
      EnsureFunctionTemplateInstantiationQueued(
          parser, template_definition, existing, completed_args);
    }
    TypeRecordDelete(func);
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return existing;
  }

  Symbol* symbol = NewSymbol(templ->name.value, func, templ->storage);
  symbol->location = templ->location;
  symbol->namespace_ = templ->namespace_;
  func->info.function.symbol = symbol;
  func->info.function.template_origin = templ;
  func->template_arguments = TemplateArgumentVectorCopy(completed_args);
  SymbolSetCXXMangledAsmName(symbol);
  existing = FindFunctionTemplateInstantiationByAsmName(templ,
                                                       symbol->asm_name.value);
  if (existing != NULL) {
    if (compiler->speculative_template_instantiation_depth == 0) {
      EnsureFunctionTemplateInstantiationQueued(
          parser, template_definition, existing, completed_args);
    }
    SymbolDelete(symbol);
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return existing;
  }
  if (PendingTemplateInstantiationHasAsmName(symbol->asm_name.value)) {
    symbol->type->info.function.definition = true;
    symbol->flags.is_defined = true;
    AppendFunctionTemplateInstantiation(templ, symbol);
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return symbol;
  }
  VectorDestruct(&symbol->attributes);
  AttributeListClone(&symbol->attributes, &templ->attributes);
  // Register this instantiation in the template's cache *before* cloning its
  // body.  A recursive function template (e.g. an introsort/merge-sort helper
  // that calls itself on subranges) contains a call to the very specialization
  // being instantiated; cloning the body triggers that call, which re-enters
  // here.  If the symbol were not yet recorded, the lookups above would miss it
  // and we would instantiate an endless chain of identical specializations
  // until the stack overflows.  Recording it first lets the recursive call
  // resolve to this in-progress symbol and terminate.
  AppendFunctionTemplateInstantiation(templ, symbol);
  if (template_definition->type->info.function.body != NULL &&
      compiler->speculative_template_instantiation_depth == 0) {
    FunctionInstantiationInProgress in_progress;
    PushFunctionInstantiationInProgress(&in_progress, symbol);
    Vector* body_args =
        FunctionTemplateBodyArguments(template_definition, symbol,
                                      completed_args);
    symbol->type->info.function.body =
        CloneTemplateFunctionBody(parser, template_definition->type,
                                  symbol->type, body_args);
    PopFunctionInstantiationInProgress(&in_progress);
    // A member function *template* constructor has its member-initializer
    // preamble intentionally deferred from class instantiation (see
    // QueueTemplateMemberFunctionDefinitionImpl); insert it here now that the
    // member's own template arguments are concrete, or its base/member
    // subobjects (and any constexpr evaluation of them) would be skipped.
    SyntaxInsertClonedTemplateConstructorPreamble(parser, template_definition,
                                                  symbol, body_args);
    if (body_args != completed_args) {
      VectorDeleteWithContents(body_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    ReanalyzeDeferredDependentAssignments(parser, symbol->type);
    symbol->type->info.function.definition = true;
    symbol->flags.is_defined = true;
    if (symbol->type->info.function.is_inline) {
      symbol->flags.is_inline_defn = true;
      if (!StorageIs(symbol->storage, STO(static))) {
        symbol->flags.is_weak = true;
      }
    }
    Vector* declarations = NewVector();
    VectorAppend(declarations,
                 NewVariableDeclarationASTNode(symbol, NULL, symbol->location));
    VectorAppend(&compiler->pending_template_instantiations,
                 NewDeclarationListASTNode(declarations, symbol->location));
    VectorAppend(&compiler->declaration_asts, symbol->type->info.function.body);
  }
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return symbol;
}

static TypeRecord* CopyDeducedTypeSpine(TypeRecord* type) {
  if (type == NULL) {
    return NULL;
  }
  TypeRecord* copy = TypeRecordCopy(type);
  if (type->next != NULL) {
    TypeRecordDelete(copy->next);
    copy->next = CopyDeducedTypeSpine(type->next);
    TypeRecordIncRef(copy->next);
  }
  return TypeRecordCalculateSize(copy);
}

/* Create a type template argument holding an independent copy of a deduced
 * type.  Deduced array/reference spines must not share element records with the
 * argument expression because deduction temporaries are destroyed earlier. */
static TemplateArgument* NewDeducedTypeTemplateArgument(TypeRecord* type) {
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  memset(arg, 0, sizeof(*arg));
  arg->kind = kTemplateParameterType;
  arg->is_pack_expansion = false;
  arg->type = CopyDeducedTypeSpine(type);
  arg->int_value = 0;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NULL;
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

/* Create a non-type template argument holding a deduced integer value. */
static TemplateArgument* NewDeducedNonTypeTemplateArgument(long long value) {
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  memset(arg, 0, sizeof(*arg));
  arg->kind = kTemplateParameterNonType;
  arg->is_pack_expansion = false;
  arg->type = NULL;
  arg->int_value = value;
  arg->value_kind = kTemplateValueIntegral;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NULL;
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

/* The deduced type for a parameter `T` from an argument: a top-level cv-stripped
 * copy of the actual type (cv-qualifiers are not deduced into a bare `T`). */
static TypeRecord* FunctionTemplateDeductionActualType(TypeRecord* actual) {
  if (actual == NULL) {
    return NULL;
  }
  TypeRecord* deduced = CopyDeducedTypeSpine(actual);
  deduced->qualifiers = kQualPlain;
  return deduced;
}

/* Record a deduced type for template parameter `index`. Explicitly supplied
 * arguments (index < explicit_arg_count) are kept as-is; otherwise set the
 * deduced type, or verify consistency if it was already deduced elsewhere. */
static bool SetDeducedFunctionTemplateTypeArgumentImpl(Vector* args,
                                                       size_t explicit_arg_count,
                                                       int index,
                                                       TypeRecord* actual,
                                                       bool strip_top_level_cv) {
  if (index < 0 || (size_t)index >= args->length) {
    return false;
  }
  if ((size_t)index < explicit_arg_count) {
    return true;
  }

  TypeRecord* deduced = strip_top_level_cv
                            ? FunctionTemplateDeductionActualType(actual)
                            : CopyDeducedTypeSpine(actual);
  TemplateArgument* existing = args->value.p[index];
  if (existing == NULL) {
    args->value.p[index] = NewDeducedTypeTemplateArgument(deduced);
    TypeRecordDelete(deduced);
    return true;
  }
  bool same = existing->kind == kTemplateParameterType &&
              existing->type != NULL &&
              TypeEqual(existing->type, deduced);
  TypeRecordDelete(deduced);
  return same;
}

static bool SetDeducedFunctionTemplateTypeArgument(Vector* args,
                                                   size_t explicit_arg_count,
                                                   int index,
                                                   TypeRecord* actual) {
  return SetDeducedFunctionTemplateTypeArgumentImpl(
      args, explicit_arg_count, index, actual, /*strip_top_level_cv=*/true);
}

static bool SetDeducedFunctionTemplateTypeArgumentPreserveQualifiers(
    Vector* args, size_t explicit_arg_count, int index, TypeRecord* actual) {
  return SetDeducedFunctionTemplateTypeArgumentImpl(
      args, explicit_arg_count, index, actual, /*strip_top_level_cv=*/false);
}

static bool AppendDeducedFunctionTemplatePackElement(
    Vector* args, size_t explicit_arg_count, int pack_index, TypeRecord* formal,
    TypeRecord* actual) {
  if (pack_index < 0 || args == NULL || (size_t)pack_index >= args->length) {
    return false;
  }
  TemplateArgument* pack = args->value.p[pack_index];
  if (pack == NULL) {
    pack = NewEmptyPackTemplateArgument(kTemplateParameterType);
    args->value.p[pack_index] = pack;
  }
  if (pack->kind != kTemplateParameterType || pack->pack_arguments == NULL) {
    return false;
  }

  Vector* element_args = TemplateArgumentVectorCopy(args);
  TemplateArgumentDelete(element_args->value.p[pack_index]);
  element_args->value.p[pack_index] = NULL;
  bool ok = DeduceFunctionTemplateTypeArgument(element_args, explicit_arg_count,
                                               formal, actual);
  TemplateArgument* element = ok ? element_args->value.p[pack_index] : NULL;
  ok = ok && element != NULL && element->kind == kTemplateParameterType &&
       element->pack_arguments == NULL;
  if (ok) {
    for (size_t i = 0; i < element_args->length; i++) {
      if (i == (size_t)pack_index) {
        continue;
      }
      TemplateArgument* deduced = element_args->value.p[i];
      TemplateArgument* existing = args->value.p[i];
      if (deduced == NULL) {
        continue;
      }
      if (existing == NULL) {
        args->value.p[i] = TemplateArgumentCopy(deduced);
      } else if (!TemplateArgumentEqual(existing, deduced)) {
        ok = false;
        break;
      }
    }
  }
  if (ok) {
    VectorAppend(pack->pack_arguments, TemplateArgumentCopy(element));
  }
  VectorDeleteWithContents(element_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return ok;
}

/* [temp.deduct.call]: when a function parameter is not a reference, an array
 * argument is replaced by the array-to-pointer result and a function argument
 * by the function-to-pointer result before deduction.  Returns a freshly
 * allocated decayed type with one reference held (release it with
 * TypeRecordDelete), or NULL when no decay applies and the caller should deduce
 * against the argument's own type.  Top-level cv-qualifiers on the decayed
 * pointer are dropped, as the standard requires. */
static TypeRecord* DecayCallArgumentTypeForDeduction(TypeRecord* formal,
                                                     TypeRecord* actual) {
  if (formal == NULL || actual == NULL) {
    return NULL;
  }
  // Only a by-value bare template parameter (`T first`) triggers decay: that is
  // the case whose deduced argument must become a pointer.  When the parameter
  // is a pointer, array, or reference the existing structural matching already
  // deduces correctly (e.g. `T*` vs an array recurses element-wise, and an
  // array parameter `char[N]` in aggregate CTAD must keep matching structurally
  // to deduce N), so decaying there would wrongly defeat deduction.
  bool bare_template_parameter = formal->declarator == kDeclPrimitive &&
                                 TypeIsUnknown(formal) &&
                                 formal->template_parameter_index >= 0;
  if (!bare_template_parameter) {
    return NULL;
  }
  TypeRecord* decayed = NULL;
  if (actual->declarator == kDeclArray) {
    decayed = NewPointerTo(kQualPlain, TypeRecordCopy(actual->next));
  } else if (actual->declarator == kDeclFunction) {
    decayed = NewPointerTo(kQualPlain, TypeRecordCopy(actual));
  }
  if (decayed != NULL) {
    TypeRecordCalculateSize(decayed);
    TypeRecordIncRef(decayed);
  }
  return decayed;
}

/* Deduce a pack element from a call argument expression, applying forwarding-
 * reference rules: a `T&&` pack parameter binding an lvalue deduces `T&`
 * (reference collapsing); otherwise deduce from the argument's type. */
static bool DeduceFunctionTemplatePackCallArgument(
    Vector* args, size_t explicit_arg_count, int pack_index, TypeRecord* formal,
    ASTNode* actual) {
  if (formal == NULL || actual == NULL || actual->type == NULL) {
    return false;
  }
  TypeRecord* target = TypeIsReference(formal) ? formal->next : formal;
  if (TypeIsCXXInitializerList(target) && actual->op != AST_OP(braced_init) &&
      !TypeIsCXXInitializerList(TypeIsReference(actual->type)
                                    ? actual->type->next
                                    : actual->type)) {
    return false;
  }
  if (actual->op == AST_OP(braced_init) &&
      !TypeIsCXXInitializerList(target) &&
      (target == NULL || target->declarator != kDeclArray)) {
    return false;
  }
  int placeholder_index = -1;
  if (formal->declarator == kDeclRValueReference &&
      TypeIsTemplateParameterPlaceholder(formal->next, &placeholder_index) &&
      actual->value_category == kValueCategoryLvalue) {
    TypeRecord* lvalue_ref = NewReferenceTypeRecord(kQualPlain, false);
    TypeRecordChain(lvalue_ref, actual->type);
    lvalue_ref->type = actual->type->type;
    TypeRecordCalculateSize(lvalue_ref);
    if (placeholder_index == pack_index) {
      TemplateArgument* pack = args->value.p[pack_index];
      if (pack == NULL) {
        pack = NewEmptyPackTemplateArgument(kTemplateParameterType);
        args->value.p[pack_index] = pack;
      }
      bool ok = pack->kind == kTemplateParameterType &&
                pack->pack_arguments != NULL;
      if (ok) {
        VectorAppend(pack->pack_arguments,
                     NewDeducedTypeTemplateArgument(lvalue_ref));
      }
      TypeRecordDelete(lvalue_ref);
      return ok;
    }
    bool ok = AppendDeducedFunctionTemplatePackElement(
        args, explicit_arg_count, pack_index, formal, lvalue_ref);
    TypeRecordDelete(lvalue_ref);
    return ok;
  }
  TypeRecord* decayed = DecayCallArgumentTypeForDeduction(formal, actual->type);
  if (decayed != NULL) {
    bool ok = AppendDeducedFunctionTemplatePackElement(
        args, explicit_arg_count, pack_index, formal, decayed);
    TypeRecordDelete(decayed);
    return ok;
  }
  return AppendDeducedFunctionTemplatePackElement(
      args, explicit_arg_count, pack_index, formal, actual->type);
}

/* Record a deduced non-type (integer) argument for parameter `index`, keeping
 * explicit args, or verifying consistency with a prior deduction. */
static bool SetDeducedFunctionTemplateNonTypeArgument(Vector* args,
                                                      size_t explicit_arg_count,
                                                      int index,
                                                      long long value) {
  if (index < 0 || (size_t)index >= args->length) {
    return false;
  }
  if ((size_t)index < explicit_arg_count) {
    return true;
  }
  TemplateArgument* existing = args->value.p[index];
  if (existing == NULL) {
    args->value.p[index] = NewDeducedNonTypeTemplateArgument(value);
    return true;
  }
  return existing->kind == kTemplateParameterNonType &&
         existing->int_value == value;
}

static bool SetDeducedTemplateNonTypeArgument(
    Vector* args, size_t explicit_arg_count, int index,
    TemplateArgument* value) {
  if (index < 0 || (size_t)index >= args->length || value == NULL ||
      value->kind != kTemplateParameterNonType) {
    return false;
  }
  if ((size_t)index < explicit_arg_count) {
    TemplateArgument* existing = args->value.p[index];
    return existing == NULL || TemplateArgumentEqual(existing, value);
  }
  TemplateArgument* existing = args->value.p[index];
  if (existing == NULL) {
    args->value.p[index] = TemplateArgumentCopy(value);
    return true;
  }
  return TemplateArgumentEqual(existing, value);
}

static bool SetDeducedTemplateTemplateArgument(
    Vector* args, size_t explicit_arg_count, int index,
    TemplateArgument* value) {
  if (index < 0 || args == NULL || (size_t)index >= args->length ||
      value == NULL || value->kind != kTemplateParameterTemplate) {
    return false;
  }
  if ((size_t)index < explicit_arg_count) {
    TemplateArgument* existing = args->value.p[index];
    return existing == NULL || TemplateArgumentEqual(existing, value);
  }
  TemplateArgument* existing = args->value.p[index];
  if (existing == NULL) {
    args->value.p[index] = TemplateArgumentCopy(value);
    return true;
  }
  return TemplateArgumentEqual(existing, value);
}

/* Unwrap a braced-initializer element to its underlying expression. */
static ASTNode* CXXBracedInitializerElementExpression(ASTNode* init) {
  if (init == NULL) {
    return NULL;
  }
  if (init->op == AST_OP(expr_init)) {
    ExpressionInitializerASTNode* expr_init =
        (ExpressionInitializerASTNode*)init;
    return expr_init->expr;
  }
  return init;
}

/* Deduce template arguments when a `T[N]`/nested-array parameter is matched
 * against a braced initializer: deduce the bound `N` from the element count and
 * the element type from each initializer (recursing for nested braces). */
static bool DeduceFunctionTemplateArrayInitializerArgument(
    Vector* args, size_t explicit_arg_count, TypeRecord* formal,
    ASTNode* actual) {
  if (actual == NULL || actual->op != AST_OP(braced_init) || formal == NULL ||
      formal->declarator != kDeclArray) {
    return false;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)actual;
  if (formal->info.array.template_parameter_index >= 0 &&
      !SetDeducedFunctionTemplateNonTypeArgument(
          args, explicit_arg_count, formal->info.array.template_parameter_index,
          (long long)braced->initializers->length)) {
    return false;
  }
  if (formal->info.array.template_parameter_index < 0 &&
      !formal->info.array.is_flexible && !formal->info.array.is_vla &&
      formal->info.array.size.fixed < (int)braced->initializers->length) {
    return false;
  }
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* init = braced->initializers->value.p[i];
    if (init == NULL) {
      return false;
    }
    if (init->op == AST_OP(braced_init)) {
      if (!DeduceFunctionTemplateArrayInitializerArgument(
              args, explicit_arg_count, formal->next, init)) {
        return false;
      }
      continue;
    }
    ASTNode* element = CXXBracedInitializerElementExpression(init);
    if (element == NULL) {
      return false;
    }
    element = AnalyzeExpression(element);
    if (!DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                            formal->next, element->type)) {
      return false;
    }
  }
  return true;
}

/* Deduce template arguments when a `std::initializer_list<T>` parameter is
 * matched against a braced initializer: deduce `T` from each element. */
static bool DeduceFunctionTemplateInitializerListArgument(
    Vector* args, size_t explicit_arg_count, TypeRecord* formal,
    ASTNode* actual) {
  TypeRecord* target = TypeIsReference(formal) ? formal->next : formal;
  if (actual == NULL || actual->op != AST_OP(braced_init) ||
      !TypeIsCXXInitializerList(target)) {
    return false;
  }
  TypeRecord* element_type = TypeCXXInitializerListElement(target);
  if (element_type == NULL) {
    return false;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)actual;
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* element =
        CXXBracedInitializerElementExpression(braced->initializers->value.p[i]);
    if (element == NULL) {
      return false;
    }
    element = AnalyzeExpression(element);
    if (!DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                            element_type, element->type)) {
      return false;
    }
  }
  return true;
}

/* Deduce template arguments when both formal and actual are instantiations of
 * the same class template (e.g. formal `Wrapper<T>` vs actual `Wrapper<int>`):
 * match them argument-by-argument, recursing into type arguments. */
/* Match one non-pack formal class-template argument against one actual argument,
 * deducing any function-template parameters it mentions into `args`. */
static bool DeduceFunctionTemplateOneTemplateArgument(Vector* args,
                                                      size_t explicit_arg_count,
                                                      TemplateArgument* formal_arg,
                                                      TemplateArgument* actual_arg) {
  if (formal_arg == NULL || actual_arg == NULL ||
      formal_arg->kind != actual_arg->kind) {
    return false;
  }
  if (formal_arg->kind == kTemplateParameterType) {
    int index = -1;
    if (TypeIsTemplateParameterPlaceholder(formal_arg->type, &index)) {
      return SetDeducedFunctionTemplateTypeArgumentPreserveQualifiers(
          args, explicit_arg_count, index, actual_arg->type);
    }
    return DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                              formal_arg->type,
                                              actual_arg->type);
  }
  if (formal_arg->kind == kTemplateParameterTemplate) {
    if (formal_arg->template_parameter_index >= 0) {
      return SetDeducedTemplateTemplateArgument(
          args, explicit_arg_count, formal_arg->template_parameter_index,
          actual_arg);
    }
    return TemplateArgumentEqual(formal_arg, actual_arg);
  }
  if (formal_arg->template_parameter_index >= 0) {
    return SetDeducedTemplateNonTypeArgument(
        args, explicit_arg_count, formal_arg->template_parameter_index,
        actual_arg);
  }
  return TemplateArgumentEqual(formal_arg, actual_arg);
}

// The template-argument list describing a class-template specialization type.
// A type record obtained from an expression -- notably the type of a
// CTAD-deduced variable used as a further deduction source (`Pair p{...}; Pair
// q = p;`) -- may not carry `template_arguments` on the record itself, but the
// specialization's tag symbol always records them.  Recover from the tag so
// deduction against a same-template argument still sees the concrete arguments.
static Vector* SpecializationTemplateArguments(TypeRecord* type) {
  if (type == NULL) {
    return NULL;
  }
  if (type->template_arguments != NULL) {
    return type->template_arguments;
  }
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL &&
      type->info.struct_info->tag_symbol != NULL &&
      type->info.struct_info->tag_symbol->type != NULL) {
    return type->info.struct_info->tag_symbol->type->template_arguments;
  }
  return NULL;
}

// The class template that `type` (a class specialization) was instantiated from,
// or NULL if `type` is not a class-template specialization.
static Symbol* ClassTemplateOriginOf(TypeRecord* type) {
  if (type == NULL) {
    return NULL;
  }
  Symbol* origin = type->template_origin;
  if (origin == NULL && TypeIsStructOrUnion(type) &&
      type->info.struct_info != NULL &&
      type->info.struct_info->tag_symbol != NULL &&
      type->info.struct_info->tag_symbol->type != NULL) {
    origin = type->info.struct_info->tag_symbol->type->template_origin;
  }
  return origin;
}

static bool ClassTemplateOriginMatches(Symbol* a, Symbol* b) {
  if (a == NULL || b == NULL) {
    return false;
  }
  return a == b || StringEqualString(&a->name, &b->name);
}

// [temp.deduct.call]/4.3: when the parameter is a class template specialization
// and the argument is derived from a specialization of that same template,
// deduction proceeds against the base class.  Searches `actual`'s base classes
// (transitively) for a base that is a specialization of `formal_origin`'s
// template and returns that base's concrete type.  Returns NULL when there is no
// such base, or more than one distinct matching base (an ambiguous case in which
// deduction fails).
static TypeRecord* FindTemplateBaseForDeduction(TypeRecord* actual,
                                                Symbol* formal_origin) {
  if (actual == NULL || !TypeIsStructOrUnion(actual) ||
      actual->info.struct_info == NULL || formal_origin == NULL) {
    return NULL;
  }
  Struct* str = actual->info.struct_info;
  TypeRecord* found = NULL;
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL) {
      continue;
    }
    TypeRecord* candidate = NULL;
    if (ClassTemplateOriginMatches(ClassTemplateOriginOf(base->type),
                                   formal_origin)) {
      candidate = base->type;
    } else {
      candidate = FindTemplateBaseForDeduction(base->type, formal_origin);
    }
    if (candidate != NULL) {
      if (found != NULL && found != candidate && !TypeEqual(found, candidate)) {
        return NULL;
      }
      found = candidate;
    }
  }
  return found;
}

static bool TemplateArgumentHasDependentMemberName(TemplateArgument* arg) {
  if (arg == NULL) {
    return false;
  }
  if (arg->pack_arguments != NULL) {
    for (size_t i = 0; i < arg->pack_arguments->length; i++) {
      if (TemplateArgumentHasDependentMemberName(
              arg->pack_arguments->value.p[i])) {
        return true;
      }
    }
    return false;
  }
  for (TypeRecord* type = arg->type; type != NULL; type = type->next) {
    if (type->dependent_member_name != NULL) {
      return true;
    }
  }
  return false;
}

static bool DeduceFunctionTemplateTemplateArguments(Vector* args,
                                                    size_t explicit_arg_count,
                                                    TypeRecord* formal,
                                                    TypeRecord* actual) {
  if (formal == NULL || actual == NULL) {
    return false;
  }
  Symbol* formal_origin = ClassTemplateOriginOf(formal);
  Symbol* actual_origin = ClassTemplateOriginOf(actual);
  if (formal_origin == NULL) {
    return false;
  }
  bool formal_is_template_parameter =
      formal_origin->flags.is_template_template_parameter;
  Vector* formal_args = SpecializationTemplateArguments(formal);
  if (formal_is_template_parameter) {
    if (actual_origin == NULL ||
        !TemplateTemplateParameterListsCompatible(
            formal_origin->template_template_parameters,
            TemplateParameterListForSymbol(actual_origin))) {
      return false;
    }
    TemplateArgument* deduced =
        NewTemplateTemplateArgument(actual_origin, -1);
    bool bound = SetDeducedTemplateTemplateArgument(
        args, explicit_arg_count,
        formal->template_parameter_index >= 0
            ? formal->template_parameter_index
            : formal_origin->template_parameter_index,
        deduced);
    TemplateArgumentDelete(deduced);
    if (!bound) {
      return false;
    }
  } else if (actual_origin == NULL ||
             !ClassTemplateOriginMatches(formal_origin, actual_origin)) {
    // The argument is not itself a specialization of the parameter's template.
    // If it derives from one, deduce against that (unique) base class.
    TypeRecord* base = FindTemplateBaseForDeduction(actual, formal_origin);
    if (base != NULL) {
      return DeduceFunctionTemplateTemplateArguments(args, explicit_arg_count,
                                                     formal, base);
    }
    return false;
  }
  Vector* actual_args = SpecializationTemplateArguments(actual);
  if (formal_args == NULL || actual_args == NULL) {
    return false;
  }
  Vector flat_actual_args;
  bool flattened_actual_args = false;
  VectorInit(&flat_actual_args);
  for (size_t i = 0; i < actual_args->length; i++) {
    TemplateArgument* actual_arg = actual_args->value.p[i];
    if (actual_arg != NULL && actual_arg->pack_arguments != NULL) {
      flattened_actual_args = true;
      for (size_t j = 0; j < actual_arg->pack_arguments->length; j++) {
        VectorAppend(&flat_actual_args, actual_arg->pack_arguments->value.p[j]);
      }
    } else {
      VectorAppend(&flat_actual_args, actual_arg);
    }
  }
  if (flattened_actual_args) {
    actual_args = &flat_actual_args;
  }
  size_t actual_index = 0;
  bool ok = true;
  for (size_t i = 0; i < formal_args->length; i++) {
    TemplateArgument* formal_arg = formal_args->value.p[i];
    if (formal_arg == NULL) {
      ok = false;
      break;
    }
    // A formal parameter-pack argument (e.g. `Wrapper<Types...>` matched against
    // `Wrapper<int, char>`): absorb the actual arguments that line up with it,
    // leaving enough for any fixed formal arguments that follow the pack.  Each
    // absorbed actual is deduced as one element of the enclosing function
    // template's pack parameter.
    int pack_index = -1;
    TemplateArgument* pack_pattern = formal_arg;
    if (formal_arg->pack_arguments != NULL &&
        formal_arg->pack_arguments->length == 1) {
      TemplateArgument* bundled_pattern =
          formal_arg->pack_arguments->value.p[0];
      if (bundled_pattern != NULL && bundled_pattern->is_pack_expansion) {
        pack_pattern = bundled_pattern;
      }
    }
    if (pack_pattern->is_pack_expansion &&
        pack_pattern->kind == kTemplateParameterNonType &&
        pack_pattern->template_parameter_index >= 0) {
      pack_index = pack_pattern->template_parameter_index;
      size_t trailing_formals = formal_args->length - i - 1;
      if ((size_t)pack_index >= args->length ||
          actual_args->length < actual_index + trailing_formals) {
        ok = false;
        break;
      }
      size_t pack_end = actual_args->length - trailing_formals;
      TemplateArgument* deduced_pack =
          NewEmptyPackTemplateArgument(kTemplateParameterNonType);
      for (; actual_index < pack_end; actual_index++) {
        TemplateArgument* actual_arg = actual_args->value.p[actual_index];
        if (actual_arg == NULL ||
            actual_arg->kind != kTemplateParameterNonType) {
          ok = false;
          break;
        }
        VectorAppend(deduced_pack->pack_arguments,
                     TemplateArgumentCopy(actual_arg));
      }
      if (ok) {
        TemplateArgument* existing = args->value.p[pack_index];
        if (existing == NULL ||
            (existing->kind == kTemplateParameterNonType &&
             existing->pack_arguments != NULL &&
             existing->pack_arguments->length == 0)) {
          TemplateArgumentDelete(existing);
          args->value.p[pack_index] = deduced_pack;
          deduced_pack = NULL;
        } else if (!TemplateArgumentEqual(existing, deduced_pack)) {
          ok = false;
        }
      }
      TemplateArgumentDelete(deduced_pack);
      if (!ok) {
        break;
      }
      continue;
    }
    if (formal_arg->is_pack_expansion &&
        formal_arg->kind == kTemplateParameterType &&
        formal_arg->type != NULL &&
        TypeIsTemplateParameterPlaceholder(formal_arg->type, &pack_index) &&
        pack_index >= 0) {
      size_t trailing_formals = formal_args->length - i - 1;
      if (actual_args->length < actual_index + trailing_formals) {
        ok = false;
        break;
      }
      size_t pack_end = actual_args->length - trailing_formals;
      // [temp.deduct]: the same template parameter pack may be named in more
      // than one function parameter, e.g.
      //   operator==(const variant<Types...>&, const variant<Types...>&).
      // Each occurrence must deduce a *consistent* sequence for `Types...`, not
      // concatenate onto it.  `AppendDeducedFunctionTemplatePackElement` only
      // ever appends, so remember how many elements a previous parameter
      // already deduced; if the pack is non-empty here we compare the freshly
      // deduced tail against that prefix and collapse the duplicate instead of
      // doubling the pack.
      TemplateArgument* pack_prev = args->value.p[pack_index];
      size_t pack_prefix =
          (pack_prev != NULL && pack_prev->pack_arguments != NULL)
              ? pack_prev->pack_arguments->length
              : 0;
      for (; actual_index < pack_end; actual_index++) {
        TemplateArgument* actual_arg = actual_args->value.p[actual_index];
        if (actual_arg == NULL) {
          ok = false;
          break;
        }
        // The actual may be a still-dependent pack expansion (e.g. matching
        // visit's `variant<VisitTypes...>` parameter against a dependent
        // `variant<Types...>` actual inside another template).  Bind the formal
        // pack to that dependent pack as a unit; it will be re-expanded when the
        // surrounding template is instantiated.
        if (actual_arg->is_pack_expansion) {
          TemplateArgument* existing = args->value.p[pack_index];
          if (existing == NULL) {
            args->value.p[pack_index] = TemplateArgumentCopy(actual_arg);
          } else if (!TemplateArgumentEqual(existing, actual_arg)) {
            ok = false;
            break;
          }
          continue;
        }
        // The actual may itself be an already-bundled pack; splice its elements.
        if (actual_arg->pack_arguments != NULL) {
          for (size_t j = 0; j < actual_arg->pack_arguments->length; j++) {
            TemplateArgument* element = actual_arg->pack_arguments->value.p[j];
            if (element == NULL || element->kind != kTemplateParameterType ||
                element->type == NULL ||
                !AppendDeducedFunctionTemplatePackElement(
                    args, explicit_arg_count, pack_index, formal_arg->type,
                    element->type)) {
              ok = false;
              break;
            }
          }
          if (!ok) {
            break;
          }
          continue;
        }
        if (actual_arg->kind != kTemplateParameterType ||
            actual_arg->type == NULL ||
            !AppendDeducedFunctionTemplatePackElement(
                args, explicit_arg_count, pack_index, formal_arg->type,
                actual_arg->type)) {
          ok = false;
          break;
        }
      }
      if (!ok) {
        break;
      }
      if (pack_prefix > 0) {
        TemplateArgument* pack_now = args->value.p[pack_index];
        if (pack_now == NULL || pack_now->pack_arguments == NULL) {
          ok = false;
          break;
        }
        size_t total = pack_now->pack_arguments->length;
        size_t appended = total - pack_prefix;
        if (appended != pack_prefix) {
          ok = false;
          break;
        }
        for (size_t k = 0; k < appended; k++) {
          if (!TemplateArgumentEqual(
                  pack_now->pack_arguments->value.p[k],
                  pack_now->pack_arguments->value.p[pack_prefix + k])) {
            ok = false;
            break;
          }
        }
        if (!ok) {
          break;
        }
        for (size_t k = total; k > pack_prefix; k--) {
          TemplateArgumentDelete(pack_now->pack_arguments->value.p[k - 1]);
          VectorPop(pack_now->pack_arguments);
        }
      }
      continue;
    }
    if (actual_index >= actual_args->length ||
        !DeduceFunctionTemplateOneTemplateArgument(
            args, explicit_arg_count, formal_arg,
            actual_args->value.p[actual_index])) {
      ok = false;
      break;
    }
    actual_index++;
  }
  ok = ok && actual_index == actual_args->length;
  VectorDestruct(&flat_actual_args);
  return ok;
}

/* True if any non-static data member of `str` still has a template-dependent
 * type (so the struct itself is dependent). */
bool StructContainsTemplateParameter(Struct* str) {
  if (str == NULL) {
    return false;
  }
  // A lambda closure with no captures has no data members, so its dependence on
  // an enclosing template parameter lives entirely in its `operator()`
  // signature (e.g. `[](const T&){...}`).  Such a closure must still be rebuilt
  // per instantiation so the call operator is substituted (and emitted) with
  // concrete types.  Consider a closure's non-template member-function
  // signatures; a generic lambda's `operator()` is itself a template and
  // references its own parameters, so it is excluded.
  bool is_lambda_closure =
      str->tag_symbol != NULL && str->tag_symbol->flags.invented;
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_using_declaration) {
      continue;
    }
    if (member->is_member_function &&
        !(is_lambda_closure && !member->symbol->flags.is_template)) {
      continue;
    }
    if (TypeContainsTemplateParameter(member->symbol->type)) {
      return true;
    }
  }
  return false;
}

/* Core type-against-type deduction: match a (possibly dependent) parameter type
 * `formal` against a concrete argument type `actual`, recording deduced
 * arguments into `args`. Handles dependent member typedefs, bare parameters
 * `T`, references, arrays `T[N]`, pointers, same-template instantiations, and
 * structural recursion through the type chain. Returns false on a mismatch. */
static bool DeduceFunctionTemplateTypeArgument(Vector* args,
                                               size_t explicit_arg_count,
                                               TypeRecord* formal,
                                               TypeRecord* actual) {
  if (formal == NULL || actual == NULL) {
    return false;
  }
  if (formal->template_origin != NULL &&
      formal->dependent_member_name != NULL) {
    TypeRecord* owner =
        TypeInstantiateClassTemplate(&compiler->syntax, formal->template_origin,
                                     formal->template_arguments);
    bool ok = false;
    if (owner != NULL && TypeIsStructOrUnion(owner) &&
        owner->info.struct_info != NULL) {
      StructMember* member =
          FindStructMember(owner->info.struct_info,
                           formal->dependent_member_name);
      if (member != NULL && member->symbol != NULL &&
          StorageIs(member->symbol->storage, STO(typedef))) {
        ok = DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                member->symbol->type, actual);
      }
    }
    TypeRecordDelete(owner);
    return ok;
  }
  // A qualified dependent name rooted directly at a template parameter
  // (`T::member`, e.g. `range_difference_t<R>` reducing to
  // `R::...::difference_type`) is a non-deduced context [temp.deduct.type].
  // While deferring, leave the root parameter unbound so a default template
  // argument can supply it; the bare-member extension below is only retried
  // when no default is available.  (The `Owner<T>::member` template-id form is
  // handled by the block above and never reaches here.)
  if (formal->dependent_member_name != NULL && g_deduce_defer_bare_member) {
    return true;
  }
  int bare_parameter_index = -1;
  if (TypeIsTemplateParameterPlaceholder(formal, &bare_parameter_index)) {
    return SetDeducedFunctionTemplateTypeArgument(
        args, explicit_arg_count, bare_parameter_index, actual);
  }

  if (TypeIsReference(formal)) {
    // In call deduction the argument type `actual` has already had any
    // top-level reference stripped, so matching against the formal's referent
    // is correct.  When deducing one *function type* against another
    // ([temp.deduct.funcaddr]), however, both the return type and the parameter
    // types keep their reference qualifiers, so a reference `actual` must be
    // reduced to its referent symmetrically for the referents to be matched.
    TypeRecord* actual_referent =
        TypeIsReference(actual) ? actual->next : actual;
    return DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                              formal->next, actual_referent);
  }

  if (formal->declarator == kDeclArray &&
      formal->info.array.template_parameter_index >= 0) {
    int index = formal->info.array.template_parameter_index;
    if (actual->declarator != kDeclArray || actual->info.array.is_vla ||
        actual->info.array.template_parameter_index >= 0 ||
        !SetDeducedFunctionTemplateNonTypeArgument(
            args, explicit_arg_count, index, actual->info.array.size.fixed)) {
      return false;
    }
  }

  if (formal->declarator != actual->declarator) {
    if (formal->declarator == kDeclPointer &&
        actual->declarator == kDeclArray) {
      return DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                formal->next, actual->next);
    }
    if (formal->declarator == kDeclPointer &&
        actual->declarator == kDeclFunction) {
      return DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                formal->next, actual);
    }
    return false;
  }
  switch (formal->declarator) {
    case kDeclPointer:
      return DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                formal->next,
                                                actual->next);
    case kDeclArray: {
      // This type model can store cv-qualification written on an array object
      // on the array node itself, while a parameter such as
      // `const T (&)[N]` stores it on the element node.  C++ treats those as
      // the same cv-qualified array type, so carry wrapper qualifiers into
      // private element copies before recursing.
      TypeRecord* formal_element = TypeRecordCopy(formal->next);
      TypeRecord* actual_element = TypeRecordCopy(actual->next);
      formal_element->qualifiers |= formal->qualifiers;
      actual_element->qualifiers |= actual->qualifiers;
      bool ok = DeduceFunctionTemplateTypeArgument(
          args, explicit_arg_count, formal_element, actual_element);
      TypeRecordDelete(formal_element);
      TypeRecordDelete(actual_element);
      return ok;
    }
    case kDeclMemberPointer:
      return TypeMemberPointerClass(formal) ==
                 TypeMemberPointerClass(actual) &&
             DeduceFunctionTemplateTypeArgument(
                 args, explicit_arg_count,
                 TypeMemberPointerPointeeType(formal),
                 TypeMemberPointerPointeeType(actual));
    case kDeclReference:
    case kDeclRValueReference:
      return false;
    case kDeclPrimitive:
      if (DeduceFunctionTemplateTemplateArguments(args, explicit_arg_count,
                                                  formal, actual)) {
        return true;
      }
      if (TypeIsStructOrUnion(formal) && TypeIsStructOrUnion(actual) &&
          (TypeContainsTemplateParameter(formal) ||
           StructContainsTemplateParameter(formal->info.struct_info))) {
        return DeduceFunctionTemplateStructMembers(args, explicit_arg_count,
                                                   formal, actual);
      }
      return TypeEqual(formal, actual);
    case kDeclFunction:
      if (!DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                              formal->next, actual->next)) {
        return false;
      }
      size_t actual_index = 0;
      for (size_t i = 0; i < formal->info.function.prototype.length; i++) {
        Symbol* formal_arg = formal->info.function.prototype.value.p[i];
        if (formal_arg == NULL) {
          return false;
        }
        if (formal_arg->flags.is_parameter_pack) {
          int pack_index = -1;
          if (!TypeIsTemplateParameterPlaceholder(formal_arg->type,
                                                  &pack_index) ||
              pack_index < 0) {
            return false;
          }
          size_t trailing =
              formal->info.function.prototype.length - i - 1;
          if (actual->info.function.prototype.length <
              actual_index + trailing) {
            return false;
          }
          size_t pack_end =
              actual->info.function.prototype.length - trailing;
          for (; actual_index < pack_end; actual_index++) {
            Symbol* actual_arg =
                actual->info.function.prototype.value.p[actual_index];
            if (actual_arg == NULL ||
                !AppendDeducedFunctionTemplatePackElement(
                    args, explicit_arg_count, pack_index,
                    formal_arg->type, actual_arg->type)) {
              return false;
            }
          }
          continue;
        }
        if (actual_index >= actual->info.function.prototype.length) {
          return false;
        }
        Symbol* actual_arg =
            actual->info.function.prototype.value.p[actual_index++];
        if (actual_arg == NULL ||
            !DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                formal_arg->type,
                                                actual_arg->type)) {
          return false;
        }
      }
      return actual_index == actual->info.function.prototype.length;
  }
  return false;
}

/* Deduce template arguments for one (non-pack) call argument expression against
 * formal parameter type `formal`, applying the forwarding-reference rule: a
 * `T&&` parameter binding an lvalue deduces `T&` (reference collapsing). */
static bool DeduceFunctionTemplateCallArgument(Vector* args,
                                               size_t explicit_arg_count,
                                               TypeRecord* formal,
                                               ASTNode* actual) {
  if (formal == NULL || actual == NULL || actual->type == NULL) {
    return false;
  }
  TypeRecord* target = TypeIsReference(formal) ? formal->next : formal;
  /* An `initializer_list<U>` parameter deduces `U` either from a braced-init
   * argument (`{1, 2, 3}`) or from an argument that is already an
   * `initializer_list<V>` (e.g. forwarding a bound `init` parameter through a
   * second template); reject only arguments that are neither. */
  if (TypeIsCXXInitializerList(target) && actual->op != AST_OP(braced_init) &&
      !TypeIsCXXInitializerList(TypeIsReference(actual->type)
                                    ? actual->type->next
                                    : actual->type)) {
    return false;
  }
  if (actual->op == AST_OP(braced_init) &&
      !TypeIsCXXInitializerList(target) &&
      (target == NULL || target->declarator != kDeclArray)) {
    return false;
  }
  /* A string literal may initialize an array of characters, so a parameter of
   * type `CharT[N]` deduces its bound `N` (and, in aggregate CTAD, its element
   * type) directly from the literal.  The literal itself has a `const`-qualified
   * element type (`const char[M]`), which need not match the (possibly
   * non-const) parameter element, so deduce against a copy of the literal's
   * array type whose element cv-qualifiers are adjusted to the parameter's. */
  if ((actual->op == AST_OP(string) || actual->op == AST_OP(string_wide)) &&
      target != NULL && target->declarator == kDeclArray &&
      target->next != NULL && actual->type != NULL &&
      actual->type->declarator == kDeclArray && actual->type->next != NULL) {
    // Copy the whole array->element chain so the adjustment below never mutates
    // the shared element type of the original string-literal expression.
    TypeRecord* adjusted = TypeRecordCopy(actual->type);
    TypeRecord* element = TypeRecordCopy(actual->type->next);
    element->qualifiers = target->next->qualifiers;
    TypeRecordIncRef(element);
    // `adjusted->next` still aliases the original (shared) element; drop that
    // borrowed reference before repointing at our private copy.
    TypeRecordDelete(adjusted->next);
    adjusted->next = element;
    TypeRecordCalculateSize(adjusted);
    TypeRecordIncRef(adjusted);
    bool ok = DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                 target, adjusted);
    TypeRecordDelete(adjusted);
    return ok;
  }
  /* [temp.deduct.call]: if the parameter type P contains no template
   * parameters, no deduction is performed from this argument.  The pairing
   * trivially succeeds; whether the argument is convertible to P is decided
   * later during overload resolution.  Requiring an exact match here would
   * wrongly reject calls needing a standard conversion (e.g. int -> long) and
   * corrupt deduction of the other, dependent, parameters. */
  if (!TypeContainsTemplateParameter(formal)) {
    return true;
  }
  /* [temp.deduct.type]: a type nominated by a qualified-id whose
   * nested-name-specifier names a member of a dependent type -- e.g.
   * `typename iterator_traits<It>::difference_type` -- is a non-deduced
   * context.  We still *attempt* deduction from it (this compiler can often
   * recover the parameter from the argument's concrete nested type, an
   * extension relied upon by e.g. `T f(typename Owner<T>::Inner)`), but a
   * *failure* to deduce here must not fail the whole call: the parameter may
   * be deduced from another argument or supplied explicitly (e.g. std::next's
   * defaulted `difference_type` second parameter).  We look through leading
   * references and pointers so that `const X<T>::type&` and `X<T>::type*` are
   * recognized too, but not `T*`/`T&` (ordinary deduced contexts, which lack a
   * dependent member name). */
  bool non_deduced_member = false;
  for (TypeRecord* p = formal; p != NULL; p = p->next) {
    if (p->dependent_member_name != NULL) {
      non_deduced_member = true;
      break;
    }
    if (p->declarator != kDeclReference && p->declarator != kDeclRValueReference &&
        p->declarator != kDeclPointer) {
      break;
    }
  }
  Vector* specialization_args = SpecializationTemplateArguments(formal);
  for (size_t i = 0;
       !non_deduced_member && specialization_args != NULL &&
       i < specialization_args->length;
       i++) {
    non_deduced_member = TemplateArgumentHasDependentMemberName(
        specialization_args->value.p[i]);
  }
  int placeholder_index = -1;
  if (formal->declarator == kDeclRValueReference &&
      TypeIsTemplateParameterPlaceholder(formal->next, &placeholder_index) &&
      actual->value_category == kValueCategoryLvalue) {
    TypeRecord* lvalue_ref = NewReferenceTypeRecord(kQualPlain, false);
    TypeRecordChain(lvalue_ref, actual->type);
    lvalue_ref->type = actual->type->type;
    TypeRecordCalculateSize(lvalue_ref);
    bool ok = DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                 formal->next, lvalue_ref);
    TypeRecordDelete(lvalue_ref);
    return ok || non_deduced_member;
  }
  TypeRecord* decayed = DecayCallArgumentTypeForDeduction(formal, actual->type);
  if (decayed != NULL) {
    bool ok = DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                 formal, decayed);
    TypeRecordDelete(decayed);
    return ok || non_deduced_member;
  }
  return DeduceFunctionTemplateTypeArgument(args, explicit_arg_count, formal,
                                            actual->type) ||
         non_deduced_member;
}

/* True for the members that contribute to structural template-argument
 * deduction: non-static data members and nested types.  Member functions and
 * static members never yield a deduced argument, and their presence differs
 * between a fully-instantiated specialization (which has synthesized implicit
 * special members) and a still-dependent primary, so they are ignored. */
static bool StructMemberParticipatesInDeduction(StructMember* member) {
  return member != NULL && member->symbol != NULL &&
         !member->is_member_function && !member->is_static;
}

/* Deduce template arguments by matching two structurally-identical structs
 * member-by-member (used for aggregate deduction): the non-static data members
 * (and nested types) must agree in order, kind, and name, and each data
 * member's type is deduced recursively. */
static bool DeduceFunctionTemplateStructMembers(Vector* args,
                                                size_t explicit_arg_count,
                                                TypeRecord* formal,
                                                TypeRecord* actual) {
  if (!TypeIsStructOrUnion(formal) || !TypeIsStructOrUnion(actual) ||
      formal->info.struct_info == NULL || actual->info.struct_info == NULL) {
    return false;
  }
  Struct* formal_struct = formal->info.struct_info;
  Struct* actual_struct = actual->info.struct_info;

  // Only non-static data members (and nested types) participate in deduction;
  // member functions and static members never contribute a deduced argument.
  // The two sides may legitimately differ in their member-function/static-member
  // sets: a concrete specialization that has already been fully instantiated has
  // its implicit special members (default/copy/move constructor, destructor,
  // assignment operators) synthesized, whereas the still-dependent primary the
  // formal side comes from does not.  So walk only the deduction-relevant
  // members on each side, in order, and ignore the rest -- comparing raw member
  // counts (or positions) would spuriously fail after such synthesis.
  size_t fi = 0;
  size_t ai = 0;
  for (;;) {
    while (fi < formal_struct->members.length &&
           !StructMemberParticipatesInDeduction(
               formal_struct->members.value.p[fi])) {
      fi++;
    }
    while (ai < actual_struct->members.length &&
           !StructMemberParticipatesInDeduction(
               actual_struct->members.value.p[ai])) {
      ai++;
    }
    bool formal_done = fi >= formal_struct->members.length;
    bool actual_done = ai >= actual_struct->members.length;
    if (formal_done && actual_done) {
      break;
    }
    if (formal_done != actual_done) {
      return false;
    }
    StructMember* formal_member = formal_struct->members.value.p[fi];
    StructMember* actual_member = actual_struct->members.value.p[ai];
    if (StructMemberIsNestedType(formal_member) !=
            StructMemberIsNestedType(actual_member) ||
        !StringEqualString(&formal_member->symbol->name,
                           &actual_member->symbol->name)) {
      return false;
    }
    if (StructMemberIsNestedType(formal_member)) {
      if (!TypeEqual(formal_member->symbol->type, actual_member->symbol->type)) {
        return false;
      }
    } else if (!DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                   formal_member->symbol->type,
                                                   actual_member->symbol->type)) {
      return false;
    }
    fi++;
    ai++;
  }
  return true;
}

/* True if every still-undeduced template parameter has a usable default
 * (type default or default int value), so deduction can be completed. */
static bool FunctionTemplateCanCompleteDeducedArguments(TypeRecord* func,
                                                        Vector* args) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.template_parameters.length != args->length) {
    return false;
  }
  for (size_t i = 0; i < args->length; i++) {
    if (args->value.p[i] != NULL) {
      continue;
    }
    TemplateParameter* param = func->info.function.template_parameters.value.p[i];
    if (param->kind == kTemplateParameterType) {
      if (param->default_type == NULL) {
        return false;
      }
    } else if (!param->has_default_int &&
               param->default_argument == NULL) {
      return false;
    }
  }
  return true;
}

/* Build the initial deduction-argument vector for a function template, one slot
 * per template parameter. Explicitly provided arguments are placed (a trailing
 * parameter pack absorbs all remaining explicit args), and parameters left to
 * be deduced are NULL. Reports how many leading slots are explicit. Returns
 * NULL on an explicit-argument/parameter mismatch. */
static Vector* NewFunctionTemplateDeductionArguments(TypeRecord* func,
                                                     Vector* explicit_args,
                                                     size_t* explicit_arg_count) {
  Vector* args = NewVector();
  size_t explicit_index = 0;
  size_t fixed_explicit_count = 0;
  for (size_t i = 0; i < func->info.function.template_parameters.length; i++) {
    TemplateParameter* param =
        func->info.function.template_parameters.value.p[i];
    if (param != NULL && param->is_parameter_pack) {
      TemplateArgument* pack = NewEmptyPackTemplateArgument(param->kind);
      while (explicit_args != NULL && explicit_index < explicit_args->length) {
        TemplateArgument* explicit_arg = explicit_args->value.p[explicit_index++];
        if (explicit_arg == NULL || explicit_arg->kind != param->kind) {
          TemplateArgumentDelete(pack);
          VectorDeleteWithContents(args,
                                   (VectorElementDestructor)TemplateArgumentDelete,
                                   /*free_element=*/false);
          return NULL;
        }
        if (explicit_arg->pack_arguments != NULL) {
          for (size_t j = 0; j < explicit_arg->pack_arguments->length; j++) {
            VectorAppend(pack->pack_arguments,
                         TemplateArgumentCopy(
                             explicit_arg->pack_arguments->value.p[j]));
          }
        } else {
          VectorAppend(pack->pack_arguments, TemplateArgumentCopy(explicit_arg));
        }
      }
      VectorAppend(args, pack);
      continue;
    }

    TemplateArgument* explicit_arg = NULL;
    if (explicit_args != NULL && explicit_index < explicit_args->length) {
      TemplateArgument* source = explicit_args->value.p[explicit_index++];
      explicit_arg = source != NULL && source->pack_arguments != NULL &&
                             source->pack_arguments->length == 1
                         ? TemplateArgumentCopy(source->pack_arguments->value.p[0])
                         : TemplateArgumentCopy(source);
    }
    if (explicit_arg != NULL) {
      fixed_explicit_count = i + 1;
    }
    VectorAppend(args, explicit_arg);
  }
  if (explicit_args != NULL && explicit_index < explicit_args->length) {
    VectorDeleteWithContents(args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return NULL;
  }
  if (explicit_arg_count != NULL) {
    *explicit_arg_count = fixed_explicit_count;
  }
  return args;
}

/* Number of leading fixed formals that must be supplied by call arguments (i.e.
 * those without a default argument), used to validate the call's arity. */
static size_t RequiredFixedFunctionTemplateFormals(TypeRecord* func,
                                                   size_t first_formal_arg,
                                                   size_t fixed_formal_count) {
  size_t required = 0;
  for (size_t i = 0; i < fixed_formal_count; i++) {
    Symbol* formal =
        func->info.function.prototype.value.p[i + first_formal_arg];
    if (formal != NULL && formal->default_argument == NULL) {
      required = i + 1;
    }
  }
  return required;
}

/* Deduce the full template argument vector for a call to function template
 * `templ` from the explicit template arguments and the actual call arguments
 * `actuals` (skipping `first_formal_arg` leading formals, e.g. an implicit
 * `this`). Validates arity (including a trailing parameter pack), deduces each
 * argument, and returns the deduced argument vector or NULL if deduction fails.
 */
static Vector* DeduceSimpleFunctionTemplateArguments(Symbol* templ,
                                                     Vector* explicit_args,
                                                     Vector* actuals,
                                                     size_t first_formal_arg) {
  if (templ == NULL || templ->type == NULL || !templ->flags.is_template ||
      !TypeIsFunction(templ->type)) {
    return NULL;
  }
  // Deduce against the candidate's current signature.  In particular, a
  // member template of an instantiated class has already had the enclosing
  // class arguments substituted and its own parameter indices rebased to
  // zero.  Falling back to value.func_defn here selects the primary class's
  // stale signature (whose member parameters still start after the enclosing
  // parameters), so deduction writes past the argument vector and rejects
  // valid calls such as a range-adaptor closure's operator()(R&&).
  TypeRecord* func = templ->type;
  if (first_formal_arg > func->info.function.prototype.length) {
    return NULL;
  }
  int formal_pack_index = -1;
  for (size_t i = first_formal_arg; i < func->info.function.prototype.length;
       i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    if (formal != NULL && formal->flags.is_parameter_pack) {
      formal_pack_index = (int)i;
      break;
    }
  }
  size_t fixed_formal_count =
      func->info.function.prototype.length - first_formal_arg;
  if (formal_pack_index >= 0) {
    fixed_formal_count = (size_t)formal_pack_index - first_formal_arg;
  }
  size_t required_formal_count = RequiredFixedFunctionTemplateFormals(
      func, first_formal_arg, fixed_formal_count);
  if (func->info.function.template_parameter_count <= 0 ||
      func->info.function.unknown_args || func->info.function.varargs ||
      (formal_pack_index < 0 &&
       (actuals->length < required_formal_count ||
        actuals->length > fixed_formal_count)) ||
      (formal_pack_index >= 0 && actuals->length < required_formal_count)) {
    return NULL;
  }
  // Deduce in two phases so a default template argument takes precedence over
  // this compiler's non-conforming `T::member` member-access deduction
  // extension: phase 1 defers bare-member parameters (leaving them for their
  // defaults); if that leaves a parameter unbound with no usable default,
  // phase 2 re-runs deduction with the extension enabled.
  bool defer_bare_member = true;
  size_t explicit_arg_count = 0;
  Vector* args = NULL;
retry_deduction:
  explicit_arg_count = 0;
  args = NewFunctionTemplateDeductionArguments(func, explicit_args,
                                               &explicit_arg_count);
  if (args == NULL) {
    return NULL;
  }
  g_deduce_defer_bare_member = defer_bare_member;
  for (size_t i = 0; i < actuals->length; i++) {
    Symbol* formal = NULL;
    if (formal_pack_index >= 0 && i >= fixed_formal_count) {
      formal = func->info.function.prototype.value.p[formal_pack_index];
      int pack_type_index = -1;
      ASTNode* actual = actuals->value.p[i];
      size_t pack_length = 0;
      if (formal == NULL || actual == NULL || actual->type == NULL ||
          !FindPackExpansionInType(formal->type, args, &pack_type_index,
                                   &pack_length) ||
          !DeduceFunctionTemplatePackCallArgument(
              args, explicit_arg_count, pack_type_index, formal->type,
              actual)) {
        g_deduce_defer_bare_member = false;
        VectorDeleteWithContents(args,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
        return NULL;
      }
      continue;
    }
    formal = func->info.function.prototype.value.p[i + first_formal_arg];
    ASTNode* actual = actuals->value.p[i];
    if (formal == NULL || actual == NULL ||
        !(DeduceFunctionTemplateArrayInitializerArgument(
              args, explicit_arg_count, formal->type, actual) ||
          DeduceFunctionTemplateInitializerListArgument(
              args, explicit_arg_count, formal->type, actual) ||
          DeduceFunctionTemplateCallArgument(args, explicit_arg_count,
                                             formal->type, actual))) {
      g_deduce_defer_bare_member = false;
      VectorDeleteWithContents(args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      return NULL;
    }
  }
  g_deduce_defer_bare_member = false;
  if (formal_pack_index >= 0) {
    Symbol* formal = func->info.function.prototype.value.p[formal_pack_index];
    int pack_type_index = -1;
    if (formal != NULL &&
        TypeIsTemplateParameterPlaceholder(formal->type, &pack_type_index) &&
        pack_type_index >= 0 && (size_t)pack_type_index < args->length &&
        args->value.p[pack_type_index] == NULL) {
      args->value.p[pack_type_index] =
          NewEmptyPackTemplateArgument(kTemplateParameterType);
    }
  }
  for (size_t i = 0; i < args->length; i++) {
    if (args->value.p[i] == NULL) {
      if (!FunctionTemplateCanCompleteDeducedArguments(func, args)) {
        VectorDeleteWithContents(args,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
        // A parameter is unbound with no usable default: retry with the
        // bare-member extension enabled before giving up.
        if (defer_bare_member) {
          defer_bare_member = false;
          goto retry_deduction;
        }
        return NULL;
      }
      break;
    }
  }
  return args;
}

/* Public: deduce and instantiate a function template from a call's actuals. */
Symbol* TypeDeduceFunctionTemplateFromCall(Syntax* syntax, Symbol* templ,
                                           Vector* actuals) {
  return TypeDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
      syntax, templ, NULL, actuals, 0);
}

/* Public: as above, with caller-supplied explicit template arguments. */
Symbol* TypeDeduceFunctionTemplateFromCallWithExplicitArgs(
    Syntax* syntax, Symbol* templ, Vector* explicit_args, Vector* actuals) {
  return TypeDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
      syntax, templ, explicit_args, actuals, 0);
}

/* Public: as above, skipping `first_formal_arg` leading formals (e.g. implicit
 * `this` for member functions). */
Symbol* TypeDeduceFunctionTemplateFromCallWithOffset(Syntax* syntax,
                                                     Symbol* templ,
                                                     Vector* actuals,
                                                     size_t first_formal_arg) {
  return TypeDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
      syntax, templ, NULL, actuals, first_formal_arg);
}

/* Public: deduce template arguments from a call (with explicit args and formal
 * offset) and instantiate the template. Returns `templ` unchanged if deduction
 * fails. */
Symbol* TypeDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
    Syntax* syntax, Symbol* templ, Vector* explicit_args, Vector* actuals,
    size_t first_formal_arg) {
  Vector* args =
      DeduceSimpleFunctionTemplateArguments(templ, explicit_args, actuals,
                                            first_formal_arg);
  if (args == NULL) {
    return templ;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  TypeRecord* func_type = templ->type;
  if (templ->is_imported_module_symbol && func_type != NULL &&
      TypeIsFunction(func_type) &&
      func_type->info.function.template_parameters.length == 0 &&
      templ->value.func_defn != NULL && templ->value.func_defn->type != NULL &&
      TypeIsFunction(templ->value.func_defn->type) &&
      templ->value.func_defn->type->info.function.template_parameters.length > 0) {
    func_type = templ->value.func_defn->type;
  }
  Vector* completed_args =
      CompleteFunctionTemplateArguments(&parser, func_type, args,
                                        /*emit_error=*/true);
  TypeParserDestruct(&parser);
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (completed_args == NULL) {
    return templ;
  }
  if (!ConceptsFunctionTemplateConstraintsSatisfied(templ, completed_args)) {
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return templ;
  }
  Symbol* symbol =
      TypeInstantiateFunctionTemplate(syntax, templ, completed_args);
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return symbol;
}

/* Public: test whether a function template's arguments can be deduced from a
 * call (used for overload viability) without instantiating it. */
bool TypeCanDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
    Symbol* templ, Vector* explicit_args, Vector* actuals,
    size_t first_formal_arg) {
  Vector* args =
      DeduceSimpleFunctionTemplateArguments(templ, explicit_args, actuals,
                                            first_formal_arg);
  if (args == NULL) {
    return false;
  }
  TypeParser parser;
  TypeParserInit(&parser, compiler->syntax.lex, &compiler->syntax,
                 STO(implicit), compiler->syntax.context);
  TypeRecord* func_type = templ->type;
  if (templ->is_imported_module_symbol && func_type != NULL &&
      TypeIsFunction(func_type) &&
      func_type->info.function.template_parameters.length == 0 &&
      templ->value.func_defn != NULL && templ->value.func_defn->type != NULL &&
      TypeIsFunction(templ->value.func_defn->type) &&
      templ->value.func_defn->type->info.function.template_parameters.length > 0) {
    func_type = templ->value.func_defn->type;
  }
  Vector* completed_args =
      CompleteFunctionTemplateArguments(&parser, func_type, args,
                                        /*emit_error=*/false);
  TypeParserDestruct(&parser);
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (completed_args == NULL) {
    return false;
  }
  bool ok = ConceptsFunctionTemplateConstraintsSatisfied(templ, completed_args);
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return ok;
}

FunctionTemplateCandidateStatus TypeClassifyFunctionTemplateCandidate(
    Syntax* syntax, Symbol* templ, Vector* explicit_args, Vector* actuals,
    size_t first_formal_arg) {
  if (templ == NULL || !templ->flags.is_template) {
    return kFunctionTemplateCandidateViable;
  }
  Vector* args =
      DeduceSimpleFunctionTemplateArguments(templ, explicit_args, actuals,
                                            first_formal_arg);
  if (args == NULL) {
    return kFunctionTemplateCandidateDeductionFailed;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  TypeRecord* func_type = templ->type;
  if (templ->is_imported_module_symbol && func_type != NULL &&
      TypeIsFunction(func_type) &&
      func_type->info.function.template_parameters.length == 0 &&
      templ->value.func_defn != NULL && templ->value.func_defn->type != NULL &&
      TypeIsFunction(templ->value.func_defn->type) &&
      templ->value.func_defn->type->info.function.template_parameters.length > 0) {
    func_type = templ->value.func_defn->type;
  }
  Vector* completed_args =
      CompleteFunctionTemplateArguments(&parser, func_type, args,
                                        /*emit_error=*/false);
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (completed_args == NULL ||
      TemplateArgumentVectorContainsTemplateParameterForInstantiation(
          completed_args)) {
    if (completed_args != NULL) {
      VectorDeleteWithContents(
          completed_args, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
    }
    TypeParserDestruct(&parser);
    return kFunctionTemplateCandidateDeductionFailed;
  }
  if (!ConceptsFunctionTemplateConstraintsSatisfied(templ, completed_args)) {
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    TypeParserDestruct(&parser);
    return kFunctionTemplateCandidateConstraintsNotSatisfied;
  }
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  TypeParserDestruct(&parser);
  return kFunctionTemplateCandidateViable;
}

/* Public: build a non-emitting concrete candidate for overload resolution.
 * This performs deduction, constraint checking, default completion, and
 * signature substitution, but it does not clone a body or enqueue codegen. */
Symbol* TypeCreateFunctionTemplateCandidate(Syntax* syntax, Symbol* templ,
                                            Vector* explicit_args,
                                            Vector* actuals,
                                            size_t first_formal_arg) {
  Vector* args =
      DeduceSimpleFunctionTemplateArguments(templ, explicit_args, actuals,
                                            first_formal_arg);
  if (args == NULL) {
    return NULL;
  }

  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  TypeRecord* func_type = templ->type;
  if (templ->is_imported_module_symbol && func_type != NULL &&
      TypeIsFunction(func_type) &&
      func_type->info.function.template_parameters.length == 0 &&
      templ->value.func_defn != NULL && templ->value.func_defn->type != NULL &&
      TypeIsFunction(templ->value.func_defn->type) &&
      templ->value.func_defn->type->info.function.template_parameters.length > 0) {
    func_type = templ->value.func_defn->type;
  }
  Vector* completed_args =
      CompleteFunctionTemplateArguments(&parser, func_type, args,
                                        /*emit_error=*/false);
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (completed_args == NULL ||
      TemplateArgumentVectorContainsTemplateParameterForInstantiation(
          completed_args)) {
    if (completed_args != NULL) {
      VectorDeleteWithContents(
          completed_args, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
    }
    TypeParserDestruct(&parser);
    return NULL;
  }
  if (!ConceptsFunctionTemplateConstraintsSatisfied(templ, completed_args)) {
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    TypeParserDestruct(&parser);
    return NULL;
  }

  Symbol* template_definition = templ;
  if ((template_definition->type == NULL ||
       template_definition->type->info.function.body == NULL) &&
      templ->value.func_defn != NULL &&
      templ->value.func_defn->type != NULL) {
    template_definition = templ->value.func_defn;
  }
  bool saved_substitution_failed = parser.template_substitution_failed;
  parser.template_substitution_failed = false;
  // Member templates of an instantiated class carry a current signature with
  // the enclosing arguments substituted and their own parameters rebased.
  // Instantiate that signature; the primary definition remains the body source.
  TypeRecord* func =
      InstantiateFunctionTemplateType(&parser, func_type, completed_args);
  bool substitution_failed = parser.template_substitution_failed;
  parser.template_substitution_failed = saved_substitution_failed;
  if (substitution_failed || TypeContainsTemplateParameter(func)) {
    TypeRecordDelete(func);
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    TypeParserDestruct(&parser);
    return NULL;
  }

  Symbol* symbol = NewSymbol(templ->name.value, func, templ->storage);
  symbol->location = templ->location;
  symbol->namespace_ = templ->namespace_;
  func->info.function.symbol = symbol;
  func->info.function.template_origin = templ;
  func->template_arguments = TemplateArgumentVectorCopy(completed_args);
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  TypeParserDestruct(&parser);
  return symbol;
}

/* Public: deduce (but do not instantiate) a function template's argument vector
 * from a call's actuals. */
Vector* TypeDeduceFunctionTemplateArgumentsFromCall(Symbol* templ,
                                                    Vector* actuals,
                                                    size_t first_formal_arg) {
  return DeduceSimpleFunctionTemplateArguments(templ, NULL, actuals,
                                               first_formal_arg);
}

/* Public: deduce the template arguments of a conversion function template for a
 * requested target type.  A conversion function template has no value
 * parameters, so its arguments are deduced by matching the declared (dependent)
 * target type (`func->next`) against the required type `target`
 * ([temp.deduct.conv]) rather than from call arguments.  Returns the completed
 * argument vector (caller owns) or NULL if deduction/default-completion/
 * constraints fail. */
Vector* TypeDeduceConversionOperatorTemplateArguments(Syntax* syntax,
                                                      Symbol* templ,
                                                      TypeRecord* target) {
  if (templ == NULL || templ->type == NULL || !templ->flags.is_template ||
      !TypeIsFunction(templ->type) || target == NULL) {
    return NULL;
  }
  TypeRecord* func =
      templ->value.func_defn != NULL && templ->value.func_defn->type != NULL
          ? templ->value.func_defn->type
          : templ->type;
  if (func->info.function.template_parameter_count <= 0 || func->next == NULL) {
    return NULL;
  }
  size_t explicit_arg_count = 0;
  Vector* args = NewFunctionTemplateDeductionArguments(func, /*explicit_args=*/
                                                       NULL, &explicit_arg_count);
  if (args == NULL) {
    return NULL;
  }
  if (!DeduceFunctionTemplateTypeArgument(args, explicit_arg_count, func->next,
                                          target)) {
    VectorDeleteWithContents(args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return NULL;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  Vector* completed_args =
      CompleteFunctionTemplateArguments(&parser, func, args,
                                        /*emit_error=*/false);
  TypeParserDestruct(&parser);
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (completed_args == NULL ||
      TemplateArgumentVectorContainsTemplateParameterForInstantiation(
          completed_args)) {
    if (completed_args != NULL) {
      VectorDeleteWithContents(completed_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    return NULL;
  }
  if (!ConceptsFunctionTemplateConstraintsSatisfied(templ, completed_args)) {
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return NULL;
  }
  return completed_args;
}

/* Public: deduce the template arguments of a function template whose *address*
 * is being taken against a required function type ([temp.deduct.funcaddr]).
 * Unlike call deduction, the template arguments are deduced by matching the
 * whole (dependent) function type of the template against the concrete target
 * function type `target_fn` (the pointee of the destination function pointer).
 * `explicit_args` supplies any explicitly-written template arguments (may be
 * NULL).  Returns the completed argument vector (caller owns) or NULL if
 * deduction / default completion / constraints fail. */
Vector* TypeDeduceFunctionTemplateArgumentsFromFunctionType(Syntax* syntax,
                                                            Symbol* templ,
                                                            Vector* explicit_args,
                                                            TypeRecord* target_fn) {
  if (templ == NULL || templ->type == NULL || !templ->flags.is_template ||
      !TypeIsFunction(templ->type) || target_fn == NULL ||
      !TypeIsFunction(target_fn)) {
    return NULL;
  }
  TypeRecord* func =
      templ->value.func_defn != NULL && templ->value.func_defn->type != NULL
          ? templ->value.func_defn->type
          : templ->type;
  if (func->info.function.template_parameter_count <= 0) {
    return NULL;
  }
  size_t explicit_arg_count = 0;
  Vector* args = NewFunctionTemplateDeductionArguments(func, explicit_args,
                                                       &explicit_arg_count);
  if (args == NULL) {
    return NULL;
  }
  if (!DeduceFunctionTemplateTypeArgument(args, explicit_arg_count, func,
                                          target_fn)) {
    VectorDeleteWithContents(args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return NULL;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  Vector* completed_args =
      CompleteFunctionTemplateArguments(&parser, func, args,
                                        /*emit_error=*/false);
  TypeParserDestruct(&parser);
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (completed_args == NULL ||
      TemplateArgumentVectorContainsTemplateParameterForInstantiation(
          completed_args)) {
    if (completed_args != NULL) {
      VectorDeleteWithContents(completed_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    return NULL;
  }
  if (!ConceptsFunctionTemplateConstraintsSatisfied(templ, completed_args)) {
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return NULL;
  }
  return completed_args;
}

/* True if conversion function template `specialized` is at least as specialized
 * as `general` for partial ordering ([temp.func.order], [temp.deduct.partial]):
 * treat `specialized`'s target pattern as the argument (its own parameters act
 * as unique opaque types on the argument side) and try to deduce `general`'s
 * parameters from it.  Success means `general` is at least as general, i.e.
 * `specialized` is at least as specialized. */
static bool ConversionTargetAtLeastAsSpecialized(Symbol* specialized,
                                                 Symbol* general) {
  TypeRecord* sfunc =
      specialized->value.func_defn != NULL &&
              specialized->value.func_defn->type != NULL
          ? specialized->value.func_defn->type
          : specialized->type;
  TypeRecord* gfunc =
      general->value.func_defn != NULL && general->value.func_defn->type != NULL
          ? general->value.func_defn->type
          : general->type;
  if (sfunc->next == NULL || gfunc->next == NULL) {
    return false;
  }
  size_t explicit_arg_count = 0;
  Vector* args =
      NewFunctionTemplateDeductionArguments(gfunc, NULL, &explicit_arg_count);
  if (args == NULL) {
    return false;
  }
  bool ok = DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                               gfunc->next, sfunc->next);
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return ok;
}

/* Public: partial ordering of two conversion function templates by their target
 * type ([temp.func.order]).  Returns 1 if `a` is more specialized than `b`, -1
 * if `b` is more specialized than `a`, and 0 if neither is (they are equivalent
 * or incomparable, i.e. ambiguous). */
int TypeConversionOperatorTemplateMoreSpecialized(Syntax* syntax, Symbol* a,
                                                  Symbol* b) {
  (void)syntax;
  if (a == NULL || b == NULL || a->type == NULL || b->type == NULL) {
    return 0;
  }
  bool a_at_least_as_specialized = ConversionTargetAtLeastAsSpecialized(a, b);
  bool b_at_least_as_specialized = ConversionTargetAtLeastAsSpecialized(b, a);
  if (a_at_least_as_specialized && !b_at_least_as_specialized) {
    return 1;
  }
  if (b_at_least_as_specialized && !a_at_least_as_specialized) {
    return -1;
  }
  return 0;
}

/* Public: register a user-written CTAD deduction guide for a class template. */
void TypeAddCXXDeductionGuide(Symbol* class_template, Symbol* guide) {
  if (class_template == NULL || class_template->type == NULL ||
      !TypeIsStructOrUnion(class_template->type) ||
      class_template->type->info.struct_info == NULL || guide == NULL) {
    return;
  }
  VectorAppend(&class_template->type->info.struct_info->deduction_guides, guide);
}

/* True if `origin` is an alias template whose pattern names another template
 * (so a use of the alias acts as a class-template placeholder for CTAD). */
static bool CXXTemplateOriginIsAliasTemplatePlaceholderOrigin(Symbol* origin) {
  return origin != NULL && StorageIs(origin->storage, STO(typedef)) &&
         origin->type != NULL && origin->type->template_origin != NULL;
}

/* True if `type` is a class-template name used without arguments (a CTAD
 * placeholder), i.e. `Foo x = ...;` where Foo is a class template, including
 * the alias-template form. */
bool TypeIsClassTemplatePlaceholder(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (TypeIsStructOrUnion(t) && t->template_origin != NULL &&
        t->info.struct_info != NULL && t->info.struct_info->is_template &&
        (t->template_arguments == NULL ||
         (CXXTemplateOriginIsAliasTemplatePlaceholderOrigin(t->template_origin) &&
          TemplateArgumentVectorContainsTemplateParameter(t->template_arguments)))) {
      return true;
    }
  }
  return false;
}

/* Return the class template a CTAD placeholder type refers to (resolving alias
 * templates to the underlying class template), or NULL. */
Symbol* TypeClassTemplatePlaceholderOrigin(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (TypeIsStructOrUnion(t) && t->template_origin != NULL &&
        t->info.struct_info != NULL && t->info.struct_info->is_template &&
        (t->template_arguments == NULL ||
         (CXXTemplateOriginIsAliasTemplatePlaceholderOrigin(t->template_origin) &&
          TemplateArgumentVectorContainsTemplateParameter(t->template_arguments)))) {
      if (CXXTemplateOriginIsAliasTemplatePlaceholderOrigin(t->template_origin)) {
        return t->template_origin->type->template_origin;
      }
      return t->template_origin;
    }
  }
  return NULL;
}

/* Return the type-chain node that is the CTAD placeholder within `type`. */
static TypeRecord* TypeClassTemplatePlaceholderBase(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (TypeIsStructOrUnion(t) && t->template_origin != NULL &&
        t->info.struct_info != NULL && t->info.struct_info->is_template &&
        (t->template_arguments == NULL ||
         (CXXTemplateOriginIsAliasTemplatePlaceholderOrigin(t->template_origin) &&
          TemplateArgumentVectorContainsTemplateParameter(t->template_arguments)))) {
      return t;
    }
  }
  return NULL;
}

static void MaxTemplateParameterIndexInType(TypeRecord* type, int* max_index) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (t->template_parameter_index > *max_index) {
      *max_index = t->template_parameter_index;
    }
    if (t->declarator == kDeclArray &&
        t->info.array.template_parameter_index > *max_index) {
      *max_index = t->info.array.template_parameter_index;
    }
    if (t->template_arguments != NULL) {
      for (size_t i = 0; i < t->template_arguments->length; i++) {
        MaxTemplateParameterIndexInArgument(t->template_arguments->value.p[i],
                                            max_index);
      }
    }
    if (TypeIsFunction(t)) {
      for (size_t i = 0; i < t->info.function.prototype.length; i++) {
        Symbol* formal = t->info.function.prototype.value.p[i];
        if (formal != NULL) {
          MaxTemplateParameterIndexInType(formal->type, max_index);
        }
      }
    }
  }
}

/* Update `*max_index` with the largest template-parameter index referenced by a
 * value-dependent argument expression (e.g. the `R` in `Box<R::num>`).  Without
 * this, an alias template whose only parameter appears solely inside a
 * dependent non-type argument expression would look parameterless and fail to
 * instantiate. */
static void MaxTemplateParameterIndexInExprVisitor(ASTNode* node, void* data,
                                                   int child_id,
                                                   VisitorMode mode) {
  (void)child_id;
  (void)mode;
  if (node == NULL || node->op != AST_OP(identifier)) {
    return;
  }
  int* max_index = (int*)data;
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol != NULL) {
    if (id->symbol->flags.is_template_parameter &&
        id->symbol->template_parameter_index > *max_index) {
      *max_index = id->symbol->template_parameter_index;
    }
    if (id->symbol->dependent_value_template_parameter_index > *max_index) {
      *max_index = id->symbol->dependent_value_template_parameter_index;
    }
    MaxTemplateParameterIndexInType(id->symbol->type, max_index);
  }
  if (id->template_arguments != NULL) {
    for (size_t i = 0; i < id->template_arguments->length; i++) {
      MaxTemplateParameterIndexInArgument(id->template_arguments->value.p[i],
                                          max_index);
    }
  }
}

void MaxTemplateParameterIndexInArgument(TemplateArgument* arg,
                                                int* max_index) {
  if (arg == NULL) {
    return;
  }
  if ((arg->kind == kTemplateParameterNonType ||
       arg->kind == kTemplateParameterTemplate) &&
      arg->template_parameter_index > *max_index) {
    *max_index = arg->template_parameter_index;
  }
  if (arg->dependent_expr != NULL) {
    ASTNodeVisit(arg->dependent_expr, MaxTemplateParameterIndexInExprVisitor, 0,
                 max_index);
  }
  if (arg->pack_arguments != NULL) {
    for (size_t i = 0; i < arg->pack_arguments->length; i++) {
      MaxTemplateParameterIndexInArgument(arg->pack_arguments->value.p[i],
                                          max_index);
    }
  }
  MaxTemplateParameterIndexInType(arg->type, max_index);
}

/* Match a partial-specialization argument `pattern` against an already-deduced
 * argument `deduced`, binding the specialization's own parameters into
 * `bindings`. Non-type params bind/compare values; type params either deduce
 * (if dependent) or require exact type equality. */
static bool TemplateArgumentPatternMatchesDeduced(Vector* bindings,
                                                  TemplateArgument* pattern,
                                                  TemplateArgument* deduced) {
  if (pattern == NULL || deduced == NULL || pattern->kind != deduced->kind) {
    return false;
  }
  if (pattern->kind == kTemplateParameterNonType) {
    if (pattern->template_parameter_index >= 0) {
      return SetDeducedTemplateNonTypeArgument(
          bindings, 0, pattern->template_parameter_index, deduced);
    }
    return TemplateArgumentEqual(pattern, deduced);
  }
  if (pattern->kind == kTemplateParameterTemplate) {
    if (pattern->template_parameter_index >= 0) {
      return SetDeducedTemplateTemplateArgument(
          bindings, 0, pattern->template_parameter_index, deduced);
    }
    return TemplateArgumentEqual(pattern, deduced);
  }
  if (pattern->type == NULL || deduced->type == NULL) {
    return false;
  }
  if (TemplateArgumentContainsTemplateParameter(pattern)) {
    return DeduceFunctionTemplateTypeArgument(bindings, 0, pattern->type,
                                              deduced->type);
  }
  return TypeEqual(pattern->type, deduced->type);
}

/* Bind class-template parameter `index` to a deduced type (or verify a prior
 * binding is consistent) when matching a partial specialization. */
static bool SetDeducedClassTemplateTypeArgument(Vector* bindings, int index,
                                                TypeRecord* actual) {
  if (index < 0 || bindings == NULL || (size_t)index >= bindings->length) {
    return false;
  }
  TemplateArgument* existing = bindings->value.p[index];
  if (existing == NULL) {
    bindings->value.p[index] = NewDeducedTypeTemplateArgument(actual);
    return true;
  }
  return existing->kind == kTemplateParameterType &&
         TypeEqual(existing->type, actual);
}

static TemplateArgument* NewDeducedTypePackTemplateArgument(void) {
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  memset(arg, 0, sizeof(*arg));
  arg->kind = kTemplateParameterType;
  arg->is_pack_expansion = false;
  arg->type = NULL;
  arg->int_value = 0;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NewVector();
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

static bool SetDeducedClassTemplateTypePackArgument(Vector* bindings, int index,
                                                    Vector* actuals,
                                                    size_t first_actual,
                                                    size_t last_actual) {
  if (index < 0 || bindings == NULL || (size_t)index >= bindings->length ||
      actuals == NULL || first_actual > actuals->length ||
      last_actual > actuals->length || first_actual > last_actual) {
    return false;
  }
  TemplateArgument* pack = NewDeducedTypePackTemplateArgument();
  for (size_t i = first_actual; i < last_actual; i++) {
    TemplateArgument* actual = actuals->value.p[i];
    if (actual == NULL) {
      TemplateArgumentDelete(pack);
      return false;
    }
    // The actual argument may itself be an already-expanded pack bundle (e.g.
    // `box<int, char, long>` stores its variadic arguments as a single pack
    // argument).  Splice that bundle's elements into the deduced pack rather
    // than nesting it, so `sizeof...` and later expansion see the individual
    // types.
    if (actual->pack_arguments != NULL) {
      for (size_t j = 0; j < actual->pack_arguments->length; j++) {
        VectorAppend(pack->pack_arguments,
                     TemplateArgumentCopy(actual->pack_arguments->value.p[j]));
      }
      continue;
    }
    if (actual->kind != kTemplateParameterType) {
      TemplateArgumentDelete(pack);
      return false;
    }
    VectorAppend(pack->pack_arguments, TemplateArgumentCopy(actual));
  }
    TemplateArgument* existing = bindings->value.p[index];
  if (existing == NULL) {
    bindings->value.p[index] = pack;
    return true;
  }
  bool equal = TemplateArgumentEqual(existing, pack);
  TemplateArgumentDelete(pack);
  return equal;
}

static bool SetDeducedClassTemplateValuePackArgument(
    Vector* bindings, int index, TemplateParameterKind kind, Vector* actuals,
    size_t first_actual, size_t last_actual) {
  if (index < 0 || bindings == NULL || (size_t)index >= bindings->length ||
      actuals == NULL || first_actual > last_actual ||
      last_actual > actuals->length) {
    return false;
  }
  TemplateArgument* pack = NewEmptyPackTemplateArgument(kind);
  for (size_t i = first_actual; i < last_actual; i++) {
    TemplateArgument* actual = actuals->value.p[i];
    if (actual == NULL || actual->kind != kind) {
      TemplateArgumentDelete(pack);
      return false;
    }
    VectorAppend(pack->pack_arguments, TemplateArgumentCopy(actual));
  }
  TemplateArgument* existing = bindings->value.p[index];
  if (existing == NULL) {
    bindings->value.p[index] = pack;
    return true;
  }
  bool equal = TemplateArgumentEqual(existing, pack);
  TemplateArgumentDelete(pack);
  return equal;
}

static bool SetDeducedClassTemplateFunctionTypePackArgument(
    Vector* bindings, int index, Vector* actual_formals, size_t first_actual,
    size_t last_actual) {
  if (index < 0 || bindings == NULL || (size_t)index >= bindings->length ||
      actual_formals == NULL || first_actual > last_actual ||
      last_actual > actual_formals->length) {
    return false;
  }
  TemplateArgument* pack = NewDeducedTypePackTemplateArgument();
  for (size_t i = first_actual; i < last_actual; i++) {
    Symbol* formal = actual_formals->value.p[i];
    if (formal == NULL || formal->type == NULL) {
      TemplateArgumentDelete(pack);
      return false;
    }
    VectorAppend(pack->pack_arguments,
                 NewDeducedTypeTemplateArgument(formal->type));
  }
  TemplateArgument* existing = bindings->value.p[index];
  if (existing == NULL) {
    bindings->value.p[index] = pack;
    return true;
  }
  bool equal = TemplateArgumentEqual(existing, pack);
  TemplateArgumentDelete(pack);
  return equal;
}

static bool TemplateArgumentIsTypeParameterPackPattern(TemplateArgument* arg,
                                                       int* index) {
  if (index != NULL) {
    *index = -1;
  }
  if (arg == NULL || arg->kind != kTemplateParameterType ||
      arg->type == NULL ||
      !TypeIsTemplateParameterPlaceholder(arg->type, index)) {
    return false;
  }
  return arg->is_pack_expansion;
}

static bool TemplateArgumentIsParameterPackPattern(TemplateArgument* arg,
                                                   int* index) {
  if (TemplateArgumentIsTypeParameterPackPattern(arg, index)) {
    return true;
  }
  if (index != NULL) {
    *index = -1;
  }
  if (arg == NULL || !arg->is_pack_expansion ||
      (arg->kind != kTemplateParameterNonType &&
       arg->kind != kTemplateParameterTemplate) ||
      arg->template_parameter_index < 0) {
    return false;
  }
  if (index != NULL) {
    *index = arg->template_parameter_index;
  }
  return true;
}

static bool ClassTemplateArgumentPackPatternMatches(Vector* bindings,
                                                    Vector* pattern_args,
                                                    Vector* actual_args) {
  if (pattern_args == NULL || actual_args == NULL) {
    return pattern_args == actual_args;
  }
  size_t actual_index = 0;
  for (size_t i = 0; i < pattern_args->length; i++) {
    TemplateArgument* pattern = pattern_args->value.p[i];
    int pack_index = -1;
    if (TemplateArgumentIsParameterPackPattern(pattern, &pack_index)) {
      if (pattern->kind == kTemplateParameterType) {
        return SetDeducedClassTemplateTypePackArgument(
            bindings, pack_index, actual_args, actual_index,
            actual_args->length);
      }
      return SetDeducedClassTemplateValuePackArgument(
          bindings, pack_index, pattern->kind, actual_args, actual_index,
          actual_args->length);
    }
    if (actual_index >= actual_args->length ||
        !ClassTemplateArgumentPatternMatches(
            bindings, pattern, actual_args->value.p[actual_index])) {
      return false;
    }
    actual_index++;
  }
  return actual_index == actual_args->length;
}

static bool PartialSpecializationBindingsContainNull(Vector* bindings) {
  if (bindings == NULL) {
    return false;
  }
  for (size_t i = 0; i < bindings->length; i++) {
    if (bindings->value.p[i] == NULL) {
      return true;
    }
  }
  return false;
}

/* Match one partial-specialization pattern argument against an actual class
 * template argument, binding the specialization's parameters into `bindings`. */
static bool ClassTemplateArgumentPatternMatches(Vector* bindings,
                                                TemplateArgument* pattern,
                                                TemplateArgument* actual) {
  if (pattern == NULL || actual == NULL || pattern->kind != actual->kind) {
    return false;
  }
  if (pattern->pack_arguments != NULL || actual->pack_arguments != NULL) {
    return pattern->pack_arguments != NULL && actual->pack_arguments != NULL &&
           ClassTemplateArgumentPackPatternMatches(bindings,
                                                   pattern->pack_arguments,
                                                   actual->pack_arguments);
  }
  int placeholder_index = -1;
  if (pattern->kind == kTemplateParameterType && pattern->type != NULL &&
      !TypeIsTemplateParameterPlaceholder(pattern->type, &placeholder_index) &&
      !PartialSpecializationBindingsContainNull(bindings)) {
    TypeParser parser;
    TypeParserInit(&parser, compiler->syntax.lex, &compiler->syntax,
                   STO(implicit), compiler->syntax.context);
    TypeRecord* substituted =
        SubstituteTemplateParameters(&parser, pattern->type, bindings);
    bool substitution_failed = parser.template_substitution_failed;
    TypeParserDestruct(&parser);
    if (substitution_failed) {
      TypeRecordDelete(substituted);
      return false;
    }
    bool ok = ClassTemplateTypePatternMatches(bindings, substituted,
                                              actual->type);
    TypeRecordDelete(substituted);
    return ok;
  }
  if (pattern->kind == kTemplateParameterNonType) {
    if (pattern->template_parameter_index >= 0) {
      return SetDeducedTemplateNonTypeArgument(
          bindings, 0, pattern->template_parameter_index, actual);
    }
    // Both arguments have already been accepted for the primary template's
    // non-type parameter, so compare integral values after that parameter's
    // implicit conversion. The expression spelling the partial pattern may
    // retain its original literal type (for example, int for `0`) while the
    // actual argument has the parameter type (for example, size_t).
    if (pattern->dependent_expr == NULL && actual->dependent_expr == NULL &&
        pattern->type != NULL && actual->type != NULL &&
        TypeIsIntegral(pattern->type) && TypeIsIntegral(actual->type)) {
      return pattern->int_value == actual->int_value;
    }
    return TemplateArgumentEqual(pattern, actual);
  }
  if (pattern->kind == kTemplateParameterTemplate) {
    if (pattern->template_parameter_index >= 0) {
      return SetDeducedTemplateTemplateArgument(
          bindings, 0, pattern->template_parameter_index, actual);
    }
    return TemplateArgumentEqual(pattern, actual);
  }
  return ClassTemplateTypePatternMatches(bindings, pattern->type, actual->type);
}

static bool ClassTemplateArgumentVectorPatternMatchesExpanded(
    Vector* bindings, Vector* pattern_args, Vector* actual_args) {
  // A template-parameter pack in the pattern (e.g. `box<T...>`) absorbs a
  // variable number of actual arguments, so the pattern and actual argument
  // counts need not match exactly. Locate a pack pattern (at most one) and let
  // it consume the middle, while leading/trailing fixed patterns match 1:1.
  int pack_pattern_pos = -1;
  for (size_t i = 0; i < pattern_args->length; i++) {
    int idx = -1;
    if (TemplateArgumentIsParameterPackPattern(pattern_args->value.p[i],
                                               &idx)) {
      pack_pattern_pos = (int)i;
      break;
    }
  }
  if (pack_pattern_pos < 0) {
    if (pattern_args->length != actual_args->length) {
      return false;
    }
    for (size_t i = 0; i < pattern_args->length; i++) {
      if (!ClassTemplateArgumentPatternMatches(
              bindings, pattern_args->value.p[i], actual_args->value.p[i])) {
        return false;
      }
    }
    return true;
  }
  size_t leading = (size_t)pack_pattern_pos;
  size_t trailing = pattern_args->length - leading - 1;
  if (actual_args->length < leading + trailing) {
    return false;
  }
  for (size_t i = 0; i < leading; i++) {
    if (!ClassTemplateArgumentPatternMatches(bindings, pattern_args->value.p[i],
                                             actual_args->value.p[i])) {
      return false;
    }
  }
  for (size_t i = 0; i < trailing; i++) {
    if (!ClassTemplateArgumentPatternMatches(
            bindings, pattern_args->value.p[leading + 1 + i],
            actual_args->value.p[actual_args->length - trailing + i])) {
      return false;
    }
  }
  int pack_index = -1;
  TemplateArgument* pack_pattern =
      pattern_args->value.p[pack_pattern_pos];
  TemplateArgumentIsParameterPackPattern(pack_pattern, &pack_index);
  if (pack_pattern->kind == kTemplateParameterType) {
    return SetDeducedClassTemplateTypePackArgument(
        bindings, pack_index, actual_args, leading,
        actual_args->length - trailing);
  }
  return SetDeducedClassTemplateValuePackArgument(
      bindings, pack_index, pack_pattern->kind, actual_args, leading,
      actual_args->length - trailing);
}

/* Match a partial-specialization argument-pattern vector against the actual
 * argument vector, accumulating parameter bindings.  Concrete variadic
 * specializations store their expanded arguments in one pack bundle; expose
 * those elements as the logical argument sequence while matching patterns such
 * as `wrapper<Head, Tail...>`. */
static bool ClassTemplateArgumentVectorPatternMatches(Vector* bindings,
                                                      Vector* pattern_args,
                                                      Vector* actual_args) {
  if (pattern_args == NULL || actual_args == NULL) {
    return pattern_args == actual_args;
  }
  // Parser-produced pack expansions are represented as a bundle in both the
  // pattern and the specialization. Preserve those corresponding bundles so
  // the nested pack matcher can bind type, non-type, or template packs. The
  // flattened path below is for patterns whose expansion remains a direct
  // argument entry.
  bool corresponding_storage =
      pattern_args->length == actual_args->length;
  for (size_t i = 0; corresponding_storage && i < pattern_args->length; i++) {
    TemplateArgument* pattern = pattern_args->value.p[i];
    TemplateArgument* actual = actual_args->value.p[i];
    corresponding_storage =
        (pattern->pack_arguments != NULL) ==
        (actual->pack_arguments != NULL);
  }
  if (corresponding_storage) {
    for (size_t i = 0; i < pattern_args->length; i++) {
      if (!ClassTemplateArgumentPatternMatches(
              bindings, pattern_args->value.p[i], actual_args->value.p[i])) {
        return false;
      }
    }
    return true;
  }
  Vector expanded_actuals;
  VectorInit(&expanded_actuals);
  for (size_t i = 0; i < actual_args->length; i++) {
    TemplateArgument* actual = actual_args->value.p[i];
    if (actual != NULL && actual->pack_arguments != NULL) {
      for (size_t j = 0; j < actual->pack_arguments->length; j++) {
        VectorAppend(&expanded_actuals, actual->pack_arguments->value.p[j]);
      }
    } else {
      VectorAppend(&expanded_actuals, actual);
    }
  }
  bool result = ClassTemplateArgumentVectorPatternMatchesExpanded(
      bindings, pattern_args, &expanded_actuals);
  VectorDestruct(&expanded_actuals);
  return result;
}

/* Match a partial-specialization type pattern (e.g. `T*`, `vector<T>`) against
 * a concrete actual type, binding `T` etc. into `bindings`. A bare parameter
 * placeholder binds the actual; otherwise structure must match exactly while
 * recursing through pointers/references/arrays/template arguments. */
static bool ClassTemplateTypePatternMatches(Vector* bindings,
                                            TypeRecord* pattern,
                                            TypeRecord* actual) {
  if (pattern == NULL || actual == NULL) {
    return pattern == actual;
  }
  int index = -1;
  if (TypeIsTemplateParameterPlaceholder(pattern, &index)) {
    if ((pattern->qualifiers & ~actual->qualifiers) != 0) {
      return false;
    }
    TypeRecord* deduced = TypeRecordCopy(actual);
    deduced->qualifiers &= ~pattern->qualifiers;
    bool ok = SetDeducedClassTemplateTypeArgument(bindings, index, deduced);
    TypeRecordDelete(deduced);
    return ok;
  }
  if (pattern->template_origin != NULL &&
      CXXTemplateOriginIsAliasTemplatePlaceholderOrigin(
          pattern->template_origin) &&
      pattern->template_arguments != NULL) {
    Vector* alias_args = CompleteAliasTemplateArguments(
        pattern->template_origin, pattern->template_arguments);
    if (alias_args == NULL) {
      return false;
    }
    TypeParser parser;
    TypeParserInit(&parser, compiler->syntax.lex, &compiler->syntax,
                   STO(implicit), compiler->syntax.context);
    TypeRecord* expanded = SubstituteTemplateParameters(
        &parser, pattern->template_origin->type, alias_args);
    bool substitution_failed = parser.template_substitution_failed;
    TypeParserDestruct(&parser);
    VectorDeleteWithContents(alias_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    bool ok =
        !substitution_failed &&
        ClassTemplateTypePatternMatches(bindings, expanded, actual);
    TypeRecordDelete(expanded);
    return ok;
  }
  if (pattern->template_origin != NULL &&
      pattern->template_origin->flags.is_template_template_parameter) {
    Symbol* actual_origin = ClassTemplateOriginOf(actual);
    Vector* pattern_args = pattern->template_arguments;
    Vector* actual_args = SpecializationTemplateArguments(actual);
    if (actual_origin == NULL || pattern_args == NULL || actual_args == NULL ||
        !TemplateTemplateParameterListsCompatible(
            pattern->template_origin->template_template_parameters,
            TemplateParameterListForSymbol(actual_origin))) {
      return false;
    }
    TemplateArgument* template_argument =
        NewTemplateTemplateArgument(actual_origin, -1);
    bool ok = SetDeducedTemplateTemplateArgument(
        bindings, 0,
        pattern->template_parameter_index >= 0
            ? pattern->template_parameter_index
            : pattern->template_origin->template_parameter_index,
        template_argument);
    TemplateArgumentDelete(template_argument);
    return ok && ClassTemplateArgumentVectorPatternMatches(
                     bindings, pattern_args, actual_args);
  }
  if (pattern->declarator != actual->declarator ||
      pattern->qualifiers != actual->qualifiers) {
    return false;
  }
  switch (pattern->declarator) {
    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
      return ClassTemplateTypePatternMatches(bindings, pattern->next,
                                             actual->next);
    case kDeclMemberPointer:
      if (pattern->template_parameter_index >= 0) {
        Struct* actual_class = TypeMemberPointerClass(actual);
        if (actual_class == NULL || actual_class->tag_symbol == NULL ||
            actual_class->tag_symbol->type == NULL ||
            !SetDeducedClassTemplateTypeArgument(
                bindings, pattern->template_parameter_index,
                actual_class->tag_symbol->type)) {
          return false;
        }
      } else if (TypeMemberPointerClass(pattern) !=
                 TypeMemberPointerClass(actual)) {
        return false;
      }
      return ClassTemplateTypePatternMatches(
          bindings, TypeMemberPointerPointeeType(pattern),
          TypeMemberPointerPointeeType(actual));
    case kDeclArray:
      if (pattern->info.array.template_parameter_index >= 0) {
        if (actual->info.array.is_vla ||
            !SetDeducedFunctionTemplateNonTypeArgument(
                bindings, 0, pattern->info.array.template_parameter_index,
                actual->info.array.size.fixed)) {
          return false;
        }
      } else if (pattern->info.array.size.fixed !=
                 actual->info.array.size.fixed) {
        return false;
      }
      return ClassTemplateTypePatternMatches(bindings, pattern->next,
                                             actual->next);
    case kDeclFunction:
      if (!ClassTemplateTypePatternMatches(bindings, pattern->next,
                                           actual->next) ||
          pattern->info.function.is_const_member !=
              actual->info.function.is_const_member ||
          pattern->info.function.is_volatile_member !=
              actual->info.function.is_volatile_member ||
          pattern->info.function.ref_qualifier !=
              actual->info.function.ref_qualifier ||
          pattern->info.function.is_noexcept !=
              actual->info.function.is_noexcept) {
        return false;
      }
      Vector* pattern_formals = &pattern->info.function.prototype;
      Vector* actual_formals = &actual->info.function.prototype;
      int pack_position = -1;
      int pack_index = -1;
      for (size_t i = 0; i < pattern_formals->length; i++) {
        Symbol* pattern_formal = pattern->info.function.prototype.value.p[i];
        int candidate_index = -1;
        if (pattern_formal != NULL &&
            pattern_formal->flags.is_parameter_pack &&
            TypeIsTemplateParameterPlaceholder(pattern_formal->type,
                                               &candidate_index)) {
          if (pack_position >= 0) {
            return false;
          }
          pack_position = (int)i;
          pack_index = candidate_index;
        }
      }
      if (pack_position < 0) {
        if (pattern_formals->length != actual_formals->length) {
          return false;
        }
        for (size_t i = 0; i < pattern_formals->length; i++) {
          Symbol* pattern_formal = pattern_formals->value.p[i];
          Symbol* actual_formal = actual_formals->value.p[i];
          if (pattern_formal == NULL || actual_formal == NULL ||
              !ClassTemplateTypePatternMatches(bindings, pattern_formal->type,
                                               actual_formal->type)) {
            return false;
          }
        }
        return true;
      }
      size_t leading = (size_t)pack_position;
      size_t trailing = pattern_formals->length - leading - 1;
      if (actual_formals->length < leading + trailing) {
        return false;
      }
      for (size_t i = 0; i < leading; i++) {
        Symbol* pattern_formal = pattern_formals->value.p[i];
        Symbol* actual_formal = actual->info.function.prototype.value.p[i];
        if (pattern_formal == NULL || actual_formal == NULL ||
            !ClassTemplateTypePatternMatches(bindings, pattern_formal->type,
                                             actual_formal->type)) {
          return false;
        }
      }
      for (size_t i = 0; i < trailing; i++) {
        Symbol* pattern_formal =
            pattern_formals->value.p[leading + 1 + i];
        Symbol* actual_formal =
            actual_formals->value.p[actual_formals->length - trailing + i];
        if (pattern_formal == NULL || actual_formal == NULL ||
            !ClassTemplateTypePatternMatches(bindings, pattern_formal->type,
                                             actual_formal->type)) {
          return false;
        }
      }
      return SetDeducedClassTemplateFunctionTypePackArgument(
          bindings, pack_index, actual_formals, leading,
          actual_formals->length - trailing);
    case kDeclPrimitive:
      if (pattern->type != actual->type) {
        return false;
      }
      if (TypeIsStructOrUnion(pattern)) {
        if (pattern->info.struct_info == actual->info.struct_info) {
          return true;
        }
        bool origin_matches = ClassTemplateOriginMatches(
            ClassTemplateOriginOf(pattern), ClassTemplateOriginOf(actual));
        if (!origin_matches) {
          return false;
        }
        bool args_match = ClassTemplateArgumentVectorPatternMatches(
            bindings, SpecializationTemplateArguments(pattern),
            SpecializationTemplateArguments(actual));
        if (!args_match) {
          return false;
        }
        return ClassTemplateOriginOf(pattern) != NULL;
      }
      if (TypeIsEnum(pattern)) {
        return pattern->info.enum_info == actual->info.enum_info;
      }
      return true;
  }
  return false;
}

/* Public: for partial CTAD (e.g. `Foo<int> x = ...`), check that a deduction
 * candidate's class template arguments are consistent with the partially
 * specified placeholder arguments by matching them as patterns. */
bool TypeClassTemplatePlaceholderAcceptsDeduced(TypeRecord* placeholder,
                                                TypeRecord* deduced) {
  TypeRecord* base = TypeClassTemplatePlaceholderBase(placeholder);
  if (base == NULL || base->template_arguments == NULL) {
    return true;
  }
  if (deduced == NULL || !TypeIsStructOrUnion(deduced)) {
    return false;
  }
  Vector* deduced_args = deduced->template_arguments;
  if (deduced_args == NULL && deduced->info.struct_info != NULL &&
      deduced->info.struct_info->tag_symbol != NULL &&
      deduced->info.struct_info->tag_symbol->type != NULL) {
    TypeRecord* tag_type = deduced->info.struct_info->tag_symbol->type;
    deduced_args = tag_type->template_arguments;
  }
  if (deduced_args == NULL ||
      deduced_args->length != base->template_arguments->length) {
    return false;
  }
  int max_index = -1;
  for (size_t i = 0; i < base->template_arguments->length; i++) {
    MaxTemplateParameterIndexInArgument(base->template_arguments->value.p[i],
                                        &max_index);
  }
  Vector bindings;
  VectorInit(&bindings);
  for (int i = 0; i <= max_index; i++) {
    VectorAppend(&bindings, NULL);
  }
  bool ok = true;
  for (size_t i = 0; ok && i < base->template_arguments->length; i++) {
    ok = TemplateArgumentPatternMatchesDeduced(
        &bindings,
        base->template_arguments->value.p[i],
        deduced_args->value.p[i]);
  }
  VectorDestructWithContents(&bindings,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  return ok;
}

/* The largest declared index among a template parameter vector. */
static int TemplateParameterVectorMaxIndex(Vector* params) {
  int max_index = -1;
  for (size_t i = 0; params != NULL && i < params->length; i++) {
    TemplateParameter* param = params->value.p[i];
    if (param != NULL && param->index > max_index) {
      max_index = param->index;
    }
  }
  return max_index;
}

/* Allocate an empty (all-NULL) bindings vector sized to hold one slot per
 * parameter of a partial specialization. */
static Vector* NewPartialSpecializationBindings(
    ClassTemplatePartialSpecialization* partial) {
  int max_index = TemplateParameterVectorMaxIndex(&partial->template_parameters);
  Vector* bindings = NewVector();
  for (int i = 0; i <= max_index; i++) {
    VectorAppend(bindings, NULL);
  }
  return bindings;
}

/* True if every (non-pack) parameter of a partial specialization received a
 * binding during pattern matching, i.e. the specialization fully applies. */
static bool PartialSpecializationBindingsComplete(
    ClassTemplatePartialSpecialization* partial, Vector* bindings) {
  for (size_t i = 0; i < partial->template_parameters.length; i++) {
    TemplateParameter* param = partial->template_parameters.value.p[i];
    if (param == NULL || param->is_parameter_pack) {
      continue;
    }
    if (param->index < 0 || (size_t)param->index >= bindings->length ||
        bindings->value.p[param->index] == NULL) {
      return false;
    }
  }
  return true;
}

/* Heuristic "specificity" score for a type pattern: a bare parameter scores 0,
 * more concrete structure (qualifiers, pointers, arrays, template arguments)
 * scores higher. Used to pick the most specialized partial specialization. */
static int TemplateTypePatternSpecificity(TypeRecord* type) {
  if (type == NULL) {
    return 0;
  }
  int placeholder_index = -1;
  if (TypeIsTemplateParameterPlaceholder(type, &placeholder_index)) {
    return 0;
  }
  int score = 1;
  if (type->qualifiers != kQualPlain) {
    score++;
  }
  switch (type->declarator) {
    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
    case kDeclMemberPointer:
      return score + 2 + TemplateTypePatternSpecificity(type->next);
    case kDeclArray:
      return score + 2 + TemplateTypePatternSpecificity(type->next);
    case kDeclFunction:
      if (type->info.function.is_const_member) {
        score++;
      }
      if (type->info.function.is_volatile_member) {
        score++;
      }
      if (type->info.function.ref_qualifier != kCXXRefQualifierNone) {
        score++;
      }
      if (type->info.function.is_noexcept) {
        score++;
      }
      score += TemplateTypePatternSpecificity(type->next);
      for (size_t i = 0; i < type->info.function.prototype.length; i++) {
        Symbol* formal = type->info.function.prototype.value.p[i];
        if (formal != NULL) {
          score += TemplateTypePatternSpecificity(formal->type);
        }
      }
      return score;
    case kDeclPrimitive:
      if (type->template_arguments != NULL) {
        for (size_t i = 0; i < type->template_arguments->length; i++) {
          TemplateArgument* arg = type->template_arguments->value.p[i];
          if (arg != NULL && arg->kind == kTemplateParameterType) {
            score += TemplateTypePatternSpecificity(arg->type);
          } else if (arg != NULL) {
            score += 2;
          }
        }
      }
      return score;
  }
  return score;
}

static int TemplateArgumentPatternSpecificity(TemplateArgument* arg) {
  if (arg == NULL) {
    return 0;
  }
  if (arg->pack_arguments != NULL) {
    return TemplateArgumentVectorPatternSpecificity(arg->pack_arguments);
  }
  if (arg->kind == kTemplateParameterType) {
    return TemplateTypePatternSpecificity(arg->type);
  }
  return arg->template_parameter_index >= 0 ? 0 : 2;
}

static void NoteSpecificityTypeParameter(int index, int* score,
                                         int* seen_type_parameters,
                                         size_t* seen_type_parameter_count) {
  for (size_t j = 0; j < *seen_type_parameter_count; j++) {
    if (seen_type_parameters[j] == index) {
      *score += 3;
      return;
    }
  }
  if (*seen_type_parameter_count < 64) {
    seen_type_parameters[(*seen_type_parameter_count)++] = index;
  }
}

static void NoteSpecificityArgumentParameters(TemplateArgument* arg, int* score,
                                              int* seen_type_parameters,
                                              size_t* seen_type_parameter_count) {
  if (arg == NULL) {
    return;
  }
  if (arg->pack_arguments != NULL) {
    for (size_t i = 0; i < arg->pack_arguments->length; i++) {
      NoteSpecificityArgumentParameters(arg->pack_arguments->value.p[i], score,
                                        seen_type_parameters,
                                        seen_type_parameter_count);
    }
    return;
  }
  if (arg->kind != kTemplateParameterType || arg->type == NULL ||
      !TypeIsTemplateParameterPlaceholder(arg->type, NULL)) {
    return;
  }
  NoteSpecificityTypeParameter(arg->type->template_parameter_index, score,
                               seen_type_parameters,
                               seen_type_parameter_count);
}

/* Total specificity score across an argument-pattern vector. */
static int TemplateArgumentVectorPatternSpecificity(Vector* args) {
  int score = 0;
  int seen_type_parameters[64];
  size_t seen_type_parameter_count = 0;
  for (size_t i = 0; args != NULL && i < args->length; i++) {
    TemplateArgument* arg = args->value.p[i];
    score += TemplateArgumentPatternSpecificity(arg);
    NoteSpecificityArgumentParameters(arg, &score, seen_type_parameters,
                                      &seen_type_parameter_count);
  }
  return score;
}

/* Try to match a single partial specialization against the actual class
 * template arguments. On success, returns its parameter bindings and a
 * specificity score (for choosing the most specialized one). */
static bool MatchClassTemplatePartialSpecialization(
    ClassTemplatePartialSpecialization* partial, Vector* actual_args,
    Vector** bindings_out, int* score_out) {
  if (partial == NULL || actual_args == NULL) {
    return false;
  }
  Vector* bindings = NewPartialSpecializationBindings(partial);
  bool ok = true;
  if (partial->pattern_arguments.length == actual_args->length) {
    for (size_t i = 0; ok && i < partial->pattern_arguments.length; i++) {
      ok = ClassTemplateArgumentPatternMatches(
          bindings, partial->pattern_arguments.value.p[i],
          actual_args->value.p[i]);
    }
  } else {
    ok = ClassTemplateArgumentVectorPatternMatches(
        bindings, &partial->pattern_arguments, actual_args);
  }
  ok = ok && PartialSpecializationBindingsComplete(partial, bindings);
  if (!ok) {
    VectorDeleteWithContents(bindings,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return false;
  }
  *bindings_out = bindings;
  *score_out =
      TemplateArgumentVectorPatternSpecificity(&partial->pattern_arguments);
  return true;
}

/* Count the parameter-pack arguments in a partial specialization's pattern.
 * Used as a partial-ordering tiebreaker: a specialization whose pattern has a
 * trailing pack (e.g. `S<I, T, Rest...>`) is less specialized than one with a
 * fixed argument list (e.g. `S<I, T>`), so when both match an argument list the
 * one with fewer packs is preferred rather than reported as ambiguous. */
static int PartialSpecializationPackCount(
    ClassTemplatePartialSpecialization* partial) {
  int count = 0;
  for (size_t i = 0; i < partial->pattern_arguments.length; i++) {
    TemplateArgument* arg = partial->pattern_arguments.value.p[i];
    if (arg != NULL && arg->is_pack_expansion) {
      count++;
    }
  }
  // A trailing template parameter pack (e.g. `class... Rest`) may match zero
  // actual arguments, in which case it leaves no pack-expansion entry in the
  // pattern.  Count such parameter packs too so that `S<I, T, Rest...>` is
  // still ranked as less specialized than `S<I, T>`.
  for (size_t i = 0; i < partial->template_parameters.length; i++) {
    TemplateParameter* param = partial->template_parameters.value.p[i];
    if (param != NULL && param->is_parameter_pack) {
      count++;
    }
  }
  return count;
}

static bool PartialSpecializationConstraintsSatisfied(
    ClassTemplatePartialSpecialization* partial, Vector* bindings) {
  return ConceptsConstraintSatisfied(partial->associated_constraint, bindings);
}

static int ComparePartialSpecializationConstraints(
    ClassTemplatePartialSpecialization* left,
    ClassTemplatePartialSpecialization* right, Vector* left_bindings,
    Vector* right_bindings) {
  return ConceptsCompareAssociatedConstraints(
      left->associated_constraint, right->associated_constraint, left_bindings,
      right_bindings);
}

static void ReportClassTemplateConstraintFailure(TypeParser* parser,
                                                 Symbol* templ,
                                                 ConstraintExpr* constraint,
                                                 Vector* arguments) {
  if (!ConceptsHasAssociatedConstraint(constraint)) {
    return;
  }
  SyntaxError(parser->syntax, "constraints not satisfied for class template %s",
              templ != NULL ? templ->name.value : "<unknown>");
  ConceptsReportAssociatedConstraintFailure(
      constraint, arguments, templ != NULL ? templ->location : SOURCE_LOCATION_MISSING,
      NULL);
}

static TypeRecord* ClassTemplateConstraintFailureType(TypeParser* parser,
                                                    Symbol* templ,
                                                    ConstraintExpr* constraint,
                                                    Vector* arguments,
                                                    bool emit_constraint_error) {
  if (emit_constraint_error) {
    ReportClassTemplateConstraintFailure(parser, templ, constraint, arguments);
  }
  return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
}

static Vector* CompleteVariableTemplateArguments(TypeParser* parser,
                                                 VariableTemplate* vt,
                                                 Vector* args,
                                                 bool emit_error) {
  if (vt == NULL) {
    return NULL;
  }
  return CompleteTemplateArguments(parser, &vt->parameters, args,
                                   "Variable template instantiation failed",
                                   emit_error);
}

static void ReportVariableTemplateConstraintFailure(Syntax* syntax,
                                                    Symbol* var_template,
                                                    Vector* arguments) {
  if (var_template == NULL || var_template->variable_template == NULL ||
      !ConceptsHasAssociatedConstraint(
          var_template->variable_template->associated_constraint)) {
    return;
  }
  SyntaxError(syntax, "constraints not satisfied for variable template %s",
              var_template->name.value);
  ConceptsReportAssociatedConstraintFailure(
      var_template->variable_template->associated_constraint, arguments,
      var_template->location, NULL);
}

static void ReportAliasTemplateConstraintFailure(TypeParser* parser,
                                                 Symbol* alias,
                                                 Vector* arguments) {
  if (!ConceptsHasAssociatedConstraint(alias->associated_constraint)) {
    return;
  }
  SyntaxError(parser->syntax, "constraints not satisfied for alias template %s",
              alias->name.value);
  ConceptsReportAssociatedConstraintFailure(alias->associated_constraint,
                                            arguments, alias->location, NULL);
}

/* Choose the best-matching partial specialization from a list (shared by class
 * and variable templates): the one with the highest specificity score. Reports
 * an ambiguity error if two equally-specific specializations match, and returns
 * NULL when none match (the primary template is then used). */
static ClassTemplatePartialSpecialization* SelectPartialSpecializationFromList(
    TypeParser* parser, Vector* specializations, const char* diagnostic_kind,
    const char* diagnostic_name, Vector* actual_args, Vector** bindings_out) {
  if (specializations == NULL) {
    return NULL;
  }
  ClassTemplatePartialSpecialization* best = NULL;
  Vector* best_bindings = NULL;
  int best_score = -1;
  int best_pack_count = 0;
  bool ambiguous = false;
  for (size_t i = 0; i < specializations->length; i++) {
    ClassTemplatePartialSpecialization* partial =
        specializations->value.p[i];
    Vector* bindings = NULL;
    int score = 0;
    if (!MatchClassTemplatePartialSpecialization(partial, actual_args,
                                                &bindings, &score)) {
      continue;
    }
    if (!PartialSpecializationConstraintsSatisfied(partial, bindings)) {
      VectorDeleteWithContents(bindings,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      continue;
    }
    int pack_count = PartialSpecializationPackCount(partial);
    // Higher specificity wins; on a tie the specialization with fewer trailing
    // parameter packs is more specialized (partial ordering) and is preferred.
    bool better = best == NULL || score > best_score ||
                  (score == best_score && pack_count < best_pack_count);
    bool tied = best != NULL && score == best_score &&
                pack_count == best_pack_count;
    if (tied) {
      int constraint_cmp = ComparePartialSpecializationConstraints(
          partial, best, bindings, best_bindings);
      if (constraint_cmp > 0) {
        better = true;
        tied = false;
      } else if (constraint_cmp < 0) {
        tied = false;
        VectorDeleteWithContents(bindings,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
        continue;
      }
    }
    if (better) {
      if (best_bindings != NULL) {
        VectorDeleteWithContents(
            best_bindings, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
      best = partial;
      best_bindings = bindings;
      best_score = score;
      best_pack_count = pack_count;
      ambiguous = false;
    } else if (tied) {
      ambiguous = true;
      VectorDeleteWithContents(bindings,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    } else {
      VectorDeleteWithContents(bindings,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
  }
  if (ambiguous) {
    SyntaxError(parser->syntax,
                "Ambiguous %s partial specialization for %s",
                diagnostic_kind != NULL ? diagnostic_kind : "template",
                diagnostic_name != NULL ? diagnostic_name : "<unknown>");
    if (best_bindings != NULL) {
      VectorDeleteWithContents(best_bindings,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    return NULL;
  }
  if (best != NULL) {
    *bindings_out = best_bindings;
  }
  return best;
}

/* Choose the best-matching partial specialization of class template `primary`
 * for the actual arguments; returns NULL when none match. */
static ClassTemplatePartialSpecialization* SelectClassTemplatePartialSpecialization(
    TypeParser* parser, Symbol* primary, Vector* actual_args,
    Vector** bindings_out) {
  if (primary == NULL || primary->type == NULL ||
      !TypeIsStructOrUnion(primary->type) ||
      primary->type->info.struct_info == NULL) {
    return NULL;
  }
  return SelectPartialSpecializationFromList(
      parser, &primary->type->info.struct_info->partial_specializations,
      "class template", primary->name.value, actual_args, bindings_out);
}

/* Choose the best-matching partial specialization of variable template
 * `primary` for the actual arguments; returns NULL when none match. */
static ClassTemplatePartialSpecialization*
SelectVariableTemplatePartialSpecialization(TypeParser* parser, Symbol* primary,
                                            Vector* actual_args,
                                            Vector** bindings_out) {
  if (primary == NULL || primary->variable_template == NULL) {
    return NULL;
  }
  return SelectPartialSpecializationFromList(
      parser, &primary->variable_template->partial_specializations,
      "variable template", primary->name.value, actual_args, bindings_out);
}

static bool TypeIsArithmeticType(TypeRecord* type) {
  return TypeIsIntegral(type) || TypeIsFloatingPoint(type);
}

static bool TypeEqualIgnoringTopLevelQualifiers(TypeRecord* left,
                                                TypeRecord* right) {
  if (left == NULL || right == NULL) {
    return false;
  }
  TypeRecord* left_plain = TypeRecordCopy(left);
  TypeRecord* right_plain = TypeRecordCopy(right);
  left_plain->qualifiers = kQualPlain;
  right_plain->qualifiers = kQualPlain;
  bool equal = TypeEqual(left_plain, right_plain);
  TypeRecordDelete(left_plain);
  TypeRecordDelete(right_plain);
  return equal;
}

static bool TypeCanAddTopLevelQualifiers(TypeRecord* from, TypeRecord* to) {
  if (!TypeEqualIgnoringTopLevelQualifiers(from, to)) {
    return false;
  }
  return (from->qualifiers & ~to->qualifiers) == 0;
}

/* Rough conversion rank for matching a CTAD deduction-guide parameter `formal`
 * against an argument type `actual` (lower is a better match, -1 = no match).
 * Used to pick the best deduction guide overload. */
static int DeductionGuideConversionRank(TypeRecord* formal,
                                        TypeRecord* actual) {
  if (formal == NULL || actual == NULL) {
    return -1;
  }
  if (TypeIsReference(formal)) {
    TypeRecord* target = formal->next;
    if (TypeEqual(target, actual)) {
      return 0;
    }
    return TypeCanAddTopLevelQualifiers(actual, target) ? 1 : -1;
  }
  if (TypeEqual(formal, actual)) {
    return 0;
  }
  if (TypeEqualIgnoringTopLevelQualifiers(formal, actual)) {
    return 1;
  }
  if (formal->declarator == kDeclPointer &&
      actual->declarator == kDeclArray &&
      TypeCanAddTopLevelQualifiers(actual->next, formal->next)) {
    return 1;
  }
  if (formal->declarator == kDeclPointer &&
      actual->declarator == kDeclFunction &&
      TypeEqual(formal->next, actual)) {
    return 1;
  }
  if (TypeEqualIgnoringSign(formal, actual)) {
    return 1;
  }
  if (TypeIsArithmeticType(formal) && TypeIsArithmeticType(actual)) {
    return 2;
  }
  if (TypeAssignmentCompatible(actual, formal)) {
    return 3;
  }
  return -1;
}

/* True if a braced initializer `{...}` can deduce a guide's
 * `std::initializer_list<T>` parameter: every element must be convertible to T. */
static bool CXXInitializerListBracedInitIsViableForDeduction(ASTNode* actual,
                                                             TypeRecord* formal) {
  TypeRecord* target = TypeIsReference(formal) ? formal->next : formal;
  if (actual == NULL || actual->op != AST_OP(braced_init) ||
      !TypeIsCXXInitializerList(target)) {
    return false;
  }
  TypeRecord* element_type = TypeCXXInitializerListElement(target);
  if (element_type == NULL) {
    return false;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)actual;
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* element =
        CXXBracedInitializerElementExpression(braced->initializers->value.p[i]);
    if (element == NULL) {
      return false;
    }
    element = AnalyzeExpression(element);
    if (!TypeAssignmentCompatible(element->type, element_type)) {
      return false;
    }
  }
  return true;
}

/* True if a braced initializer can deduce a guide's array parameter `T[N]`:
 * within bounds, every (possibly nested) element convertible to the element. */
static bool CXXArrayBracedInitIsViableForDeduction(ASTNode* actual,
                                                   TypeRecord* formal) {
  TypeRecord* target = TypeIsReference(formal) ? formal->next : formal;
  if (actual == NULL || actual->op != AST_OP(braced_init) || target == NULL ||
      target->declarator != kDeclArray) {
    return false;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)actual;
  if (!target->info.array.is_flexible && !target->info.array.is_vla &&
      target->info.array.template_parameter_index < 0 &&
      target->info.array.size.fixed < (int)braced->initializers->length) {
    return false;
  }
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* init = braced->initializers->value.p[i];
    if (init == NULL) {
      return false;
    }
    if (init->op == AST_OP(braced_init)) {
      if (!CXXArrayBracedInitIsViableForDeduction(init, target->next)) {
        return false;
      }
      continue;
    }
    ASTNode* element = CXXBracedInitializerElementExpression(init);
    if (element == NULL) {
      return false;
    }
    element = AnalyzeExpression(element);
    if (!TypeAssignmentCompatible(element->type, target->next)) {
      return false;
    }
  }
  return true;
}

/* Score a CTAD deduction guide against the constructor call's actual arguments:
 * substitute the guide's parameters with `template_args`, rank each
 * parameter/argument conversion, and sum the ranks (lower total = better).
 * Returns false if the guide is not viable for these arguments. */
static bool ScoreDeductionGuideCall(TypeParser* parser,
                                    TypeRecord* guide_type,
                                    Vector* template_args,
                                    Vector* actuals,
                                    int* score) {
  if (parser == NULL || guide_type == NULL || !TypeIsFunction(guide_type) ||
      actuals == NULL || score == NULL) {
    return false;
  }
  TypeRecord* concrete_guide =
      template_args != NULL
          ? InstantiateFunctionTemplateType(parser, guide_type, template_args)
          : guide_type;
  if (concrete_guide == NULL || !TypeIsFunction(concrete_guide) ||
      concrete_guide->info.function.prototype.length != actuals->length) {
    if (template_args != NULL) {
      TypeRecordDelete(concrete_guide);
    }
    return false;
  }
  int total = 0;
  for (size_t i = 0; i < actuals->length; i++) {
    Symbol* formal = concrete_guide->info.function.prototype.value.p[i];
    ASTNode* actual = actuals->value.p[i];
    if (formal == NULL || formal->type == NULL || actual == NULL) {
      if (template_args != NULL) {
        TypeRecordDelete(concrete_guide);
      }
      return false;
    }
    TypeRecord* formal_type = formal->type;
    int rank = actual->op == AST_OP(braced_init)
        ? (CXXArrayBracedInitIsViableForDeduction(actual, formal_type) ||
                   CXXInitializerListBracedInitIsViableForDeduction(actual,
                                                                    formal_type)
               ? 0
               : -1)
        : DeductionGuideConversionRank(formal_type, actual->type);
    if (rank < 0) {
      if (template_args != NULL) {
        TypeRecordDelete(concrete_guide);
      }
      return false;
    }
    total += rank;
  }
  if (template_args != NULL) {
    TypeRecordDelete(concrete_guide);
  }
  *score = total;
  return true;
}

static bool CXXDeductionCandidateEqual(TypeRecord* left, TypeRecord* right) {
  if (TypeIsStructOrUnion(left) || TypeIsStructOrUnion(right)) {
    return TypeIsStructOrUnion(left) && TypeIsStructOrUnion(right) &&
           left->info.struct_info == right->info.struct_info;
  }
  return TypeEqual(left, right);
}

static TypeRecord* TypeDeduceClassTemplateFromGuideFiltered(
    Syntax* syntax, Symbol* class_template, TypeRecord* placeholder,
    Vector* actuals, bool allow_explicit, bool* alias_rejected) {
  if (syntax == NULL || class_template == NULL || class_template->type == NULL ||
      !class_template->flags.is_template || actuals == NULL ||
      !TypeIsStructOrUnion(class_template->type) ||
      class_template->type->info.struct_info == NULL) {
    return NULL;
  }
  Struct* str = class_template->type->info.struct_info;
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  TypeRecord* result = NULL;
  int best_score = -1;
  bool ambiguous = false;
  bool best_is_explicit = false;
  for (size_t i = 0; i < str->deduction_guides.length; i++) {
    Symbol* guide = str->deduction_guides.value.p[i];
    if (guide == NULL || guide->type == NULL || !TypeIsFunction(guide->type)) {
      continue;
    }
    TypeRecord* guide_return = NULL;
    int guide_score = 0;
    if (guide->flags.is_template) {
      Vector* args =
          TypeDeduceFunctionTemplateArgumentsFromCall(guide, actuals, 0);
      if (args == NULL) {
        continue;
      }
      if (!ConceptsFunctionTemplateConstraintsSatisfied(guide, args)) {
        VectorDeleteWithContents(args,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
        continue;
      }
      if (!ScoreDeductionGuideCall(&parser, guide->type, args, actuals,
                                   &guide_score)) {
        VectorDeleteWithContents(args,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
        continue;
      }
      guide_return = SubstituteTemplateParameters(&parser, guide->type->next,
                                                  args);
      VectorDeleteWithContents(args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    } else {
      if (!ScoreDeductionGuideCall(&parser, guide->type, NULL, actuals,
                                   &guide_score)) {
        continue;
      }
      guide_return = TypeRecordCopy(guide->type->next);
    }
    if (guide_return == NULL) {
      continue;
    }
    if (guide->type->info.function.is_implicitly_declared) {
      guide_score += 1000;
    }
    if (guide->flags.is_template) {
      guide_score += 10;
    }
    TypeRecord* candidate = NULL;
    if (guide_return->template_origin == class_template &&
        guide_return->template_arguments != NULL) {
      Struct* class_struct = class_template->type->info.struct_info;
      if (class_struct != NULL &&
          ConceptsConstraintSatisfied(class_struct->associated_constraint,
                                    guide_return->template_arguments)) {
        candidate = TypeInstantiateClassTemplateQuiet(
            syntax, class_template, guide_return->template_arguments);
      }
    } else if (guide_return->template_origin == class_template ||
               (TypeIsStructOrUnion(guide_return) &&
                guide_return->info.struct_info != NULL &&
                !guide_return->info.struct_info->is_template)) {
      candidate = TypeRecordCopy(guide_return);
    }
    TypeRecordDelete(guide_return);
    if (candidate == NULL) {
      continue;
    }
    if (placeholder != NULL &&
        !TypeClassTemplatePlaceholderAcceptsDeduced(placeholder, candidate)) {
      if (alias_rejected != NULL) {
        *alias_rejected = true;
      }
      TypeRecordDelete(candidate);
      continue;
    }
    if (result == NULL || guide_score < best_score) {
      if (result != NULL) {
        TypeRecordDelete(result);
      }
      result = candidate;
      best_score = guide_score;
      best_is_explicit = guide->type->info.function.is_explicit;
      ambiguous = false;
    } else if (guide_score == best_score &&
               !CXXDeductionCandidateEqual(result, candidate)) {
      ambiguous = true;
      TypeRecordDelete(candidate);
    } else {
      TypeRecordDelete(candidate);
    }
  }
  TypeParserDestruct(&parser);
  if (ambiguous) {
    SyntaxError(syntax, "Ambiguous class template argument deduction for %s",
                class_template->name.value);
    if (result != NULL) {
      TypeRecordDelete(result);
    }
    return NULL;
  }
  if (result != NULL && best_is_explicit && !allow_explicit) {
    SyntaxError(syntax,
                "Explicit deduction guide cannot be used for copy-initialization");
    TypeRecordDelete(result);
    return NULL;
  }
  return result;
}

TypeRecord* TypeDeduceClassTemplateFromGuide(Syntax* syntax,
                                             Symbol* class_template,
                                             Vector* actuals,
                                             bool allow_explicit) {
  return TypeDeduceClassTemplateFromGuideFiltered(
      syntax, class_template, NULL, actuals, allow_explicit, NULL);
}

TypeRecord* TypeDeduceClassTemplateFromPlaceholder(Syntax* syntax,
                                                   TypeRecord* placeholder,
                                                   Vector* actuals,
                                                   bool allow_explicit,
                                                   bool* alias_rejected) {
  if (alias_rejected != NULL) {
    *alias_rejected = false;
  }
  if (placeholder == NULL || !TypeIsClassTemplatePlaceholder(placeholder)) {
    return NULL;
  }
  Symbol* class_template = TypeClassTemplatePlaceholderOrigin(placeholder);
  TypeRecord* alias_base = TypeClassTemplatePlaceholderBase(placeholder);
  TypeRecord* filter = alias_base != NULL && alias_base->template_arguments != NULL
                           ? placeholder
                           : NULL;
  return TypeDeduceClassTemplateFromGuideFiltered(
      syntax, class_template, filter, actuals, allow_explicit, alias_rejected);
}

/* Set up the source->target struct substitution and clone/queue the body of an
 * instantiated member function.  Factored out so it can run in a second pass,
 * after every member function signature has been added to `owner`. */
StructMember* InstantiateTemplateMemberFunction(TypeParser* parser,
                                                       Struct* owner,
                                                       StructMember* member,
                                                       Vector* args,
                                                       Vector* pending) {
  Struct* saved_substitution_source = parser->template_substitution_source;
  Struct* saved_substitution_target = parser->template_substitution_target;
  TypeRecord* source_owner =
      member->symbol != NULL && member->symbol->type != NULL
          ? member->symbol->type
                ->info.function.cxx_member_owner != NULL
                ? member->symbol->type->info.function.cxx_member_owner
                      ->tag_symbol != NULL
                      ? member->symbol->type->info.function.cxx_member_owner
                            ->tag_symbol->type
                      : NULL
                : NULL
          : NULL;
  Struct* substitution_source =
      source_owner != NULL && TypeIsStructOrUnion(source_owner)
          ? source_owner->info.struct_info
          : NULL;
  parser->template_substitution_source = substitution_source;
  parser->template_substitution_target = owner;
  TypeRecord* func = InstantiateMemberFunctionType(
      parser, owner, member->is_static, member->symbol->type, args,
      member->symbol->location);
  const char* symbol_name = member->symbol->name.value;
  String destructor_name;
  StringInit(&destructor_name, NULL);
  // A conversion operator's name is derived from its result type, so once that
  // type has been substituted the name must be recomputed: a conversion to a
  // dependent target (e.g. `operator View<C, int>`) is spelled "operator View"
  // in the primary template but must become "operator View<char,int>" in the
  // `Str<char>` instantiation, or overload resolution's conversion-operator
  // matching (which compares the member name against the name derived from its
  // result type) fails to recognise it as a conversion function at all.
  String conversion_name;
  StringInit(&conversion_name, NULL);
  bool source_is_conversion_operator = false;
  if (!func->info.function.is_constructor &&
      !func->info.function.is_destructor &&
      member->symbol->type->next != NULL) {
    String source_expected;
    ConversionOperatorName(member->symbol->type->next, &source_expected);
    source_is_conversion_operator =
        strcmp(source_expected.value, member->symbol->name.value) == 0;
    StringDestruct(&source_expected);
  }
  if (func->info.function.is_constructor && owner->tag_name != NULL) {
    symbol_name = owner->tag_name->value;
  } else if (func->info.function.is_destructor && owner->tag_name != NULL) {
    StringSet(&destructor_name, "~");
    StringAppendString(&destructor_name, owner->tag_name);
    symbol_name = destructor_name.value;
  } else if (source_is_conversion_operator && func->next != NULL) {
    // Name the instantiated conversion operator from its substituted result
    // type.  This can still be a deferred template-id when the result names a
    // class template completed later in the TU; overload resolution
    // (ClassHasConversionOperatorTo) re-materializes the result type when it
    // matches, so it does not rely on this name being fully specialized.
    ConversionOperatorName(func->next, &conversion_name);
    symbol_name = conversion_name.value;
  }
  Symbol* symbol = NewSymbol(symbol_name, func,
                             member->symbol->storage);
  StringDestruct(&destructor_name);
  StringDestruct(&conversion_name);
  symbol->location = member->symbol->location;
  symbol->flags.is_template = member->symbol->flags.is_template;
  symbol->type->info.function.template_parameter_count =
      member->symbol->type->info.function.template_parameter_count;
  symbol->type->info.function.template_parameter_base = 0;
  func->info.function.symbol = symbol;
  SymbolSetCXXMangledAsmName(symbol);
  Symbol* template_definition = member->symbol->value.func_defn;
  if (template_definition == NULL &&
      member->symbol->type->info.function.body != NULL) {
    template_definition = member->symbol;
  }
  // Keep the source definition reachable while this class specialization is
  // still being assembled. A later data member can use an earlier constexpr
  // member in a dependent template argument (for example
  // `array<T, rank_dynamic()>`); constexpr evaluation must be able to
  // instantiate that body before the normal pending-body pass runs.
  symbol->value.func_defn = template_definition;
  parser->template_substitution_source = saved_substitution_source;
  parser->template_substitution_target = saved_substitution_target;
  PendingMemberBody* pmb = malloc(sizeof(PendingMemberBody));
  pmb->symbol = symbol;
  pmb->template_definition = template_definition;
  pmb->substitution_source = substitution_source;
  VectorAppend(pending, pmb);
  StructMember* instantiated = NewStructMember(symbol);
  instantiated->is_member_function = true;
  instantiated->is_static = member->is_static;
  instantiated->access = member->access;
  return instantiated;
}

static TemplateArgument* NewDefaultTypeTemplateArgument(TypeRecord* type) {
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  memset(arg, 0, sizeof(*arg));
  arg->kind = kTemplateParameterType;
  arg->is_pack_expansion = false;
  arg->type = type;
  arg->int_value = 0;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NULL;
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

static TemplateArgument* NewDefaultNonTypeTemplateArgument(
    long long int_value, int template_parameter_index) {
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  memset(arg, 0, sizeof(*arg));
  arg->kind = kTemplateParameterNonType;
  arg->is_pack_expansion = false;
  arg->type = NULL;
  arg->int_value = int_value;
  arg->value_kind = template_parameter_index < 0
                        ? kTemplateValueIntegral : kTemplateValueNone;
  arg->template_parameter_index = template_parameter_index;
  arg->pack_arguments = NULL;
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

TemplateArgument* NewEmptyPackTemplateArgument(
    TemplateParameterKind kind) {
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  memset(arg, 0, sizeof(*arg));
  arg->kind = kind;
  arg->is_pack_expansion = false;
  arg->type = NULL;
  arg->int_value = 0;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NewVector();
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

static int FindTemplateParameterPackIndex(Vector* template_parameters) {
  for (size_t i = 0; template_parameters != NULL &&
                     i < template_parameters->length; i++) {
    TemplateParameter* param = template_parameters->value.p[i];
    if (param != NULL && param->is_parameter_pack) {
      return (int)i;
    }
  }
  return -1;
}

static bool TemplateNonTypeArgumentMatchesParameter(
    TemplateParameter* param, TemplateArgument* arg) {
  if (param == NULL || arg == NULL ||
      param->kind != kTemplateParameterNonType ||
      arg->kind != kTemplateParameterNonType) {
    return false;
  }
  // A nested template-id can be completed while its non-type argument still
  // depends on an enclosing class template, for example
  // `array<index_type, rank_dynamic()>` inside `extents<...>`.  Its concrete
  // value and type are checked after substitution; rejecting the unresolved
  // expression here prevents the enclosing specialization from ever forming.
  if (arg->dependent_expr != NULL) {
    return true;
  }
  if (param->type == NULL || (param->type->type & kTypeAuto) != 0 ||
      TypeContainsTemplateParameter(param->type)) {
    return TemplateArgumentConcreteValueKind(arg) != kTemplateValueNone ||
           arg->template_parameter_index >= 0 ||
           arg->dependent_expr != NULL;
  }
  TemplateValueKind value_kind = TemplateArgumentConcreteValueKind(arg);
  if (value_kind == kTemplateValueIntegral) {
    return param->type->declarator == kDeclPrimitive &&
           (param->type->type &
            (kTypeFloat | kTypeDouble | kTypeLongDouble |
             kTypeStruct | kTypeUnion | kTypeVoid)) == 0;
  }
  if (arg->type == NULL) {
    return false;
  }
  if (value_kind == kTemplateValueNull) {
    return TypeIsPointer(param->type) || TypeIsMemberPointer(param->type) ||
           TypeIsNullPointer(param->type);
  }
  if (value_kind == kTemplateValuePointer) {
    if (!TypeIsPointer(arg->type) || !TypeIsPointer(param->type)) {
      return false;
    }
    bool argument_is_function =
        arg->type->next != NULL && TypeIsFunction(arg->type->next);
    bool parameter_is_function =
        param->type->next != NULL && TypeIsFunction(param->type->next);
    return argument_is_function == parameter_is_function &&
           TypeAssignmentCompatible(arg->type, param->type);
  }
  if (value_kind == kTemplateValueMemberPointer) {
    return TypeEqual(arg->type, param->type);
  }
  return TypeAssignmentCompatible(arg->type, param->type);
}

static Vector* TemplateParameterListForSymbol(Symbol* symbol) {
  if (symbol == NULL || !symbol->flags.is_template) {
    return NULL;
  }
  if (symbol->flags.is_template_template_parameter) {
    return symbol->template_template_parameters;
  }
  if (symbol->alias_template != NULL) {
    return &symbol->alias_template->parameters;
  }
  if (symbol->type != NULL && TypeIsStructOrUnion(symbol->type) &&
      symbol->type->info.struct_info != NULL) {
    return &symbol->type->info.struct_info->template_parameters;
  }
  return NULL;
}

static bool TemplateParameterHasDefault(TemplateParameter* param) {
  if (param == NULL) {
    return false;
  }
  if (param->is_parameter_pack) {
    return true;
  }
  if (param->kind == kTemplateParameterType) {
    return param->default_type != NULL;
  }
  return param->default_argument != NULL || param->has_default_int;
}

static bool TemplateTemplateParameterListsCompatible(Vector* formal,
                                                       Vector* actual) {
  if (formal == NULL || actual == NULL) {
    return false;
  }
  size_t fi = 0;
  size_t ai = 0;
  while (fi < formal->length) {
    TemplateParameter* fp = formal->value.p[fi];
    if (fp == NULL) {
      return false;
    }
    if (fp->is_parameter_pack) {
      for (; ai < actual->length; ai++) {
        TemplateParameter* ap = actual->value.p[ai];
        if (ap == NULL || ap->kind != fp->kind ||
            (fp->kind == kTemplateParameterTemplate &&
             !TemplateTemplateParameterListsCompatible(
                 fp->template_parameters, ap->template_parameters))) {
          return false;
        }
      }
      return true;
    }
    if (ai >= actual->length) {
      return false;
    }
    TemplateParameter* ap = actual->value.p[ai];
    if (ap == NULL || ap->kind != fp->kind ||
        (fp->kind == kTemplateParameterNonType &&
         fp->type != NULL && ap->type != NULL &&
         (fp->type->type & kTypeAuto) == 0 &&
         (ap->type->type & kTypeAuto) == 0 &&
         !TypeEqual(fp->type, ap->type)) ||
        (fp->kind == kTemplateParameterTemplate &&
         !TemplateTemplateParameterListsCompatible(
             fp->template_parameters, ap->template_parameters))) {
      return false;
    }
    fi++;
    ai++;
  }
  for (; ai < actual->length; ai++) {
    if (!TemplateParameterHasDefault(actual->value.p[ai])) {
      return false;
    }
  }
  return true;
}

static bool TemplateTemplateArgumentMatchesParameter(
    TemplateParameter* param, TemplateArgument* arg) {
  if (param == NULL || arg == NULL ||
      param->kind != kTemplateParameterTemplate ||
      arg->kind != kTemplateParameterTemplate) {
    return false;
  }
  Vector* actual = TemplateParameterListForSymbol(arg->template_symbol);
  if (actual == NULL && arg->template_parameter_index >= 0) {
    return true;
  }
  return TemplateTemplateParameterListsCompatible(
      param->template_parameters, actual);
}

static const char* TemplateArgumentKindError(TemplateParameterKind kind) {
  if (kind == kTemplateParameterType) {
    return "Template argument must name a type";
  }
  if (kind == kTemplateParameterTemplate) {
    return "Template argument must name a compatible template";
  }
  return "Template non-type argument must be an integer constant expression";
}

/* Produce a full template argument vector with one entry per declared
 * parameter: copy supplied `args`, substitute defaults (which may themselves
 * reference earlier parameters) for any omitted trailing parameters, and gather
 * leftover args into a trailing parameter pack. Emits `error_message` and
 * returns NULL if required arguments are missing. */
static Vector* CompleteTemplateArguments(TypeParser* parser,
                                         Vector* template_parameters,
                                         Vector* args,
                                         const char* error_message,
                                         bool emit_error) {
  if (args == NULL) {
    if (emit_error) {
      SyntaxError(parser->syntax, "%s", error_message);
    }
    return NULL;
  }
  int pack_index = FindTemplateParameterPackIndex(template_parameters);
  if (pack_index < 0 && args->length > template_parameters->length) {
    if (emit_error) {
      SyntaxError(parser->syntax, "Too many template arguments");
    }
    return NULL;
  }
  Vector* completed = NewVector();
  for (size_t i = 0; i < template_parameters->length; i++) {
    TemplateParameter* param = template_parameters->value.p[i];
    if (param != NULL && param->is_parameter_pack) {
      TemplateArgument* pack = NewEmptyPackTemplateArgument(param->kind);
      // Deduction produces a normalized vector with one bundled argument per
      // declared parameter. Preserve that one-to-one shape for a non-trailing
      // pack instead of greedily absorbing the arguments belonging to later
      // parameters. Raw explicit argument lists still use the trailing-pack
      // gathering path.
      bool normalized_pack =
          args->length == template_parameters->length && i < args->length &&
          args->value.p[i] != NULL &&
          ((TemplateArgument*)args->value.p[i])->pack_arguments != NULL;
      size_t pack_end = normalized_pack ? i + 1 : args->length;
      for (size_t j = i; j < pack_end; j++) {
        TemplateArgument* arg = args->value.p[j];
        bool incompatible_template =
            arg != NULL && arg->kind == kTemplateParameterTemplate &&
            arg->pack_arguments == NULL &&
            !TemplateTemplateArgumentMatchesParameter(param, arg);
        if (arg == NULL || arg->kind != param->kind ||
            incompatible_template) {
          if (emit_error) {
            SyntaxError(
                parser->syntax, "%s",
                incompatible_template
                    ? "Template argument does not match template parameter list"
                    : TemplateArgumentKindError(param->kind));
          }
          TemplateArgumentDelete(pack);
          VectorDeleteWithContents(completed,
                                   (VectorElementDestructor)TemplateArgumentDelete,
                                   /*free_element=*/false);
          return NULL;
        }
        if (arg->pack_arguments != NULL) {
          for (size_t k = 0; k < arg->pack_arguments->length; k++) {
            TemplateArgument* element = arg->pack_arguments->value.p[k];
            bool incompatible_element =
                element != NULL &&
                element->kind == kTemplateParameterTemplate &&
                !TemplateTemplateArgumentMatchesParameter(param, element);
            if (element == NULL || element->kind != param->kind ||
                incompatible_element) {
              if (emit_error) {
                SyntaxError(
                    parser->syntax, "%s",
                    incompatible_element
                        ? "Template argument does not match template parameter list"
                        : TemplateArgumentKindError(param->kind));
              }
              TemplateArgumentDelete(pack);
              VectorDeleteWithContents(
                  completed, (VectorElementDestructor)TemplateArgumentDelete,
                  /*free_element=*/false);
              return NULL;
            }
            VectorAppend(pack->pack_arguments, TemplateArgumentCopy(element));
          }
        } else {
          VectorAppend(pack->pack_arguments, TemplateArgumentCopy(arg));
        }
      }
      VectorAppend(completed, pack);
      continue;
    }
    bool argument_from_default = false;
    TemplateArgument* arg =
        i < args->length ? TemplateArgumentCopy(args->value.p[i]) : NULL;
    if (arg == NULL) {
      if (param->kind == kTemplateParameterType &&
          param->default_type != NULL) {
        TypeRecord* default_type =
            SubstituteTemplateParameters(parser, param->default_type,
                                         completed);
        arg = NewDefaultTypeTemplateArgument(default_type);
      } else if (param->kind == kTemplateParameterNonType &&
                 (param->default_argument != NULL ||
                  param->has_default_int)) {
        argument_from_default = true;
        TemplateArgument* default_argument = param->default_argument;
        int default_template_parameter_index =
            default_argument != NULL
                ? default_argument->template_parameter_index
                : param->default_template_parameter_index;
        if (default_template_parameter_index >= 0 &&
            (size_t)default_template_parameter_index < completed->length) {
          TemplateArgument* actual =
              completed->value.p[default_template_parameter_index];
          if (actual->kind == kTemplateParameterNonType) {
            arg = TemplateArgumentCopy(actual);
          }
        }
        if (arg == NULL) {
          arg = default_argument != NULL
                    ? NewSubstitutedTemplateArgument(parser, default_argument,
                                                     completed)
                    : NewDefaultNonTypeTemplateArgument(
                          param->default_int_value,
                          default_template_parameter_index);
        }
      } else if (param->kind == kTemplateParameterTemplate &&
                 param->default_argument != NULL) {
        arg = NewSubstitutedTemplateArgument(
            parser, param->default_argument, completed);
      } else {
        if (emit_error) {
          SyntaxError(parser->syntax, "%s", error_message);
        }
        VectorDeleteWithContents(completed,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
        return NULL;
      }
    }
    if (argument_from_default &&
        param->kind == kTemplateParameterNonType &&
        param->type != NULL && (param->type->type & kTypeAuto) == 0 &&
        !TypeContainsTemplateParameter(param->type) &&
        arg != NULL && arg->dependent_expr == NULL) {
      TypeRecordDelete(arg->type);
      arg->type = TypeRecordCopy(param->type);
    }
    if (param->kind != arg->kind) {
      if (emit_error) {
        SyntaxError(parser->syntax, "%s",
                    TemplateArgumentKindError(param->kind));
      }
      TemplateArgumentDelete(arg);
      VectorDeleteWithContents(completed,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      return NULL;
    }
    if (param->kind == kTemplateParameterNonType &&
        !TemplateNonTypeArgumentMatchesParameter(param, arg)) {
      if (emit_error) {
        SyntaxError(parser->syntax,
                    "Template non-type argument is not compatible with parameter type");
      }
      TemplateArgumentDelete(arg);
      VectorDeleteWithContents(completed,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      return NULL;
    }
    if (param->kind == kTemplateParameterTemplate &&
        !TemplateTemplateArgumentMatchesParameter(param, arg)) {
      if (emit_error) {
        SyntaxError(parser->syntax,
                    "Template argument does not match template parameter list");
      }
      TemplateArgumentDelete(arg);
      VectorDeleteWithContents(completed,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      return NULL;
    }
    if (param->kind == kTemplateParameterNonType &&
        param->type != NULL && (param->type->type & kTypeAuto) == 0 &&
        !TypeContainsTemplateParameter(param->type) &&
        arg->template_parameter_index < 0 && arg->dependent_expr == NULL) {
      TypeRecordDelete(arg->type);
      arg->type = TypeRecordCopy(param->type);
    }
    VectorAppend(completed, arg);
  }
  return completed;
}

/* Complete a concept-id's argument list against the concept's own template
 * parameters, applying trailing default arguments.  Concept default arguments
 * can name earlier parameters (e.g. `C = common_type_t<T, U>`), which
 * CompleteTemplateArguments resolves by substituting the arguments filled so
 * far.  Returns a freshly owned vector or NULL; never diagnoses. */
Vector* TypeCompleteConceptArguments(Syntax* syntax, Vector* concept_parameters,
                                     Vector* args) {
  if (syntax == NULL || concept_parameters == NULL || args == NULL) {
    return NULL;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  Vector* completed = CompleteTemplateArguments(
      &parser, concept_parameters, args,
      "too few template arguments for concept", /*emit_error=*/false);
  TypeParserDestruct(&parser);
  return completed;
}

/* Complete a class template's argument list against its parameters (filling in
 * defaults and gathering a trailing pack); see CompleteTemplateArguments. */
Vector* CompleteClassTemplateArguments(TypeParser* parser,
                                              Struct* template_struct,
                                              Vector* args) {
  return CompleteTemplateArguments(parser, &template_struct->template_parameters,
                                   args,
                                   "Class template instantiation is not supported yet",
                                   /*emit_error=*/true);
}

/* Register a partial specialization (its tag and argument pattern) on the
 * primary class template, rejecting an exact duplicate pattern. */
static void AddOwnedAssociatedConstraint(ConstraintExpr** target,
                                         ConstraintExpr* constraint) {
  if (target == NULL || constraint == NULL) {
    return;
  }
  ConstraintExpr* current = *target;
  if (current == NULL) {
    *target = constraint;
    return;
  }
  *target = NewConjunctionConstraint(current, constraint, constraint->location);
}

static void MoveTemplateParameterConstraints(Vector* parameters,
                                             ConstraintExpr** target) {
  if (parameters == NULL || target == NULL) {
    return;
  }
  for (size_t i = 0; i < parameters->length; i++) {
    TemplateParameter* param = parameters->value.p[i];
    if (param == NULL || param->associated_constraint == NULL) {
      continue;
    }
    ConstraintExpr* constraint = param->associated_constraint;
    param->associated_constraint = NULL;
    AddOwnedAssociatedConstraint(target, constraint);
  }
}

void AddClassTemplatePartialSpecialization(TypeParser* parser,
                                                  Symbol* primary,
                                                  Symbol* partial_tag,
                                                  Vector* pattern_args) {
  if (primary == NULL || primary->type == NULL ||
      !TypeIsStructOrUnion(primary->type) ||
      primary->type->info.struct_info == NULL || partial_tag == NULL ||
      pattern_args == NULL) {
    return;
  }
  Struct* primary_struct = primary->type->info.struct_info;
  // Build the new specialization (including its associated constraint) up front
  // so duplicate detection can compare constraints, not just the pattern.  Two
  // specializations that share a pattern but have *distinct* constraints are
  // valid constrained partial specializations (e.g. incrementable_traits<T>
  // constrained on `has-member-difference-type` vs. `subtractable-integral`);
  // only a same-pattern, constraint-equivalent redeclaration is a duplicate.
  ClassTemplatePartialSpecialization* partial =
      NewClassTemplatePartialSpecialization(
          partial_tag, parser->syntax->current_template_parameters,
          pattern_args);
  MoveTemplateParameterConstraints(&partial->template_parameters,
                                   &partial->associated_constraint);
  if (parser->syntax->current_template_requires_clause != NULL) {
    AddOwnedAssociatedConstraint(&partial->associated_constraint,
                                 parser->syntax->current_template_requires_clause);
    parser->syntax->current_template_requires_clause = NULL;
  }
  for (size_t i = 0; i < primary_struct->partial_specializations.length; i++) {
    ClassTemplatePartialSpecialization* existing =
        primary_struct->partial_specializations.value.p[i];
    if (existing != NULL &&
        TemplateArgumentPatternVectorEqual(&existing->pattern_arguments,
                                           &partial->pattern_arguments) &&
        ConceptsAssociatedConstraintsEquivalent(
            existing->associated_constraint, &existing->template_parameters,
            partial->associated_constraint, &partial->template_parameters)) {
      SyntaxError(parser->syntax,
                  "Duplicate class template partial specialization %s",
                  partial_tag->name.value);
      ClassTemplatePartialSpecializationDelete(partial);
      return;
    }
  }
  VectorAppend(&primary_struct->partial_specializations, partial);
}

/* Register a partial/explicit specialization of a *variable* template.  The
 * specialization's parameters are the current template parameters, its pattern
 * is `pattern_args` (the template-id written after the variable name), and its
 * value is `initializer` (kept unanalyzed, folded per use). */
void AddVariableTemplatePartialSpecialization(TypeParser* parser,
                                              Symbol* primary,
                                              Vector* pattern_args,
                                              struct ASTNode* initializer,
                                              struct TypeRecord* type) {
  if (primary == NULL || primary->variable_template == NULL ||
      pattern_args == NULL) {
    if (initializer != NULL) {
      ASTNodeDelete(initializer);
    }
    return;
  }
  VariableTemplate* vt = primary->variable_template;
  for (size_t i = 0; i < vt->partial_specializations.length; i++) {
    ClassTemplatePartialSpecialization* existing =
        vt->partial_specializations.value.p[i];
    if (existing != NULL &&
        TemplateArgumentPatternVectorEqual(&existing->pattern_arguments,
                                           pattern_args)) {
      SyntaxError(parser->syntax,
                  "Duplicate variable template specialization %s",
                  primary->name.value);
      if (initializer != NULL) {
        ASTNodeDelete(initializer);
      }
      return;
    }
  }
  ClassTemplatePartialSpecialization* partial =
      NewVariableTemplatePartialSpecialization(
          parser->syntax->current_template_parameters, pattern_args,
          initializer, type);
  MoveTemplateParameterConstraints(&partial->template_parameters,
                                   &partial->associated_constraint);
  if (parser->syntax->current_template_requires_clause != NULL) {
    AddOwnedAssociatedConstraint(&partial->associated_constraint,
                                 parser->syntax->current_template_requires_clause);
    parser->syntax->current_template_requires_clause = NULL;
  }
  VectorAppend(&vt->partial_specializations, partial);
}

/* Complete a function template's argument list against its parameters (filling
 * defaults / gathering a trailing pack), then clear the "unknown/placeholder"
 * marking on type arguments that resolved to their own parameter position so
 * they read as concrete deduced types. Returns NULL if completion fails. */
static Vector* CompleteFunctionTemplateArguments(TypeParser* parser,
                                                 TypeRecord* func,
                                                 Vector* args,
                                                 bool emit_error) {
  Vector* parameters = NULL;
  if (func != NULL && TypeIsFunction(func)) {
    parameters = &func->info.function.template_parameters;
    if (parameters->length == 0 && func->info.function.symbol != NULL &&
        func->info.function.symbol->imported_function_template_parameters_backup
                .length > 0) {
      parameters =
          &func->info.function.symbol->imported_function_template_parameters_backup;
    }
  }
  if (func == NULL || !TypeIsFunction(func) || parameters == NULL ||
      parameters->length == 0) {
    if (emit_error) {
      SyntaxError(parser->syntax,
                  "Function template instantiation is not supported yet");
    }
    return NULL;
  }
  Vector* completed =
      CompleteTemplateArguments(parser, parameters, args,
                                "Function template instantiation is not supported yet",
                                emit_error);
  if (completed == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < completed->length; i++) {
    TemplateArgument* arg = completed->value.p[i];
    if (arg != NULL && arg->kind == kTemplateParameterType &&
        arg->type != NULL && TypeIsUnknown(arg->type) &&
        arg->type->template_parameter_index == (int)i) {
      arg->type->type &= ~kTypeUnknown;
      arg->type->template_parameter_index = -1;
    }
  }
  return completed;
}

/* Instantiate the deferred friend functions a class template declared, mapping
 * each onto a concrete free function in the template's namespace.  The friend's
 * signature (and inline body, if any) is substituted with the instantiation
 * arguments; the resulting function is registered for overload resolution/ADL,
 * recorded as a friend of `str`, and queued for emission when it carries a body
 * (deduplicated against earlier specializations and existing declarations). */
/* Materialize the deferred friend functions listed on `friend_owner`, recording
 * each concrete friend on `record_into` and registering it for overload
 * resolution / ADL.  Dependent signatures and inline bodies are substituted with
 * `args`, resolving any nested-type references (e.g. a sentinel's friend naming
 * the iterator type) by name through the `subst_source`->`subst_target` mapping.
 * For a template's own friends all four structs coincide with (str, source);
 * for a nested class's friends, `record_into`/`friend_owner` are the nested
 * instantiated/template structs while `subst_source`/`subst_target` remain the
 * *enclosing* template/instantiation so sibling nested types remap correctly. */
static void InstantiateTemplateFriendFunctionsImpl(TypeParser* parser,
                                                   Struct* record_into,
                                                   Struct* friend_owner,
                                                   Struct* subst_source,
                                                   Struct* subst_target,
                                                   Vector* args) {
  for (size_t i = 0; i < friend_owner->friend_functions.length; i++) {
    Symbol* ftpl = friend_owner->friend_functions.value.p[i];
    if (ftpl == NULL || ftpl->type == NULL || !TypeIsFunction(ftpl->type)) {
      if (ftpl != NULL) {
        StructAddFriendFunction(record_into, ftpl);
      }
      continue;
    }

    Struct* saved_source = parser->template_substitution_source;
    Struct* saved_target = parser->template_substitution_target;
    parser->template_substitution_source = subst_source;
    parser->template_substitution_target = subst_target;

    TypeRecord* func = InstantiateMemberFunctionType(
        parser, /*owner=*/NULL, /*is_static_member=*/true, ftpl->type, args,
        ftpl->location);
    bool constraints_satisfied =
        func->info.function.associated_constraint == NULL ||
        ConceptsConstraintSatisfied(
            func->info.function.associated_constraint, args);
    func->info.function.cxx_member_owner = NULL;

    Symbol* sym = NewSymbol(ftpl->name.value, func, ftpl->storage);
    sym->flags = ftpl->flags;
    sym->location = ftpl->location;
    sym->namespace_ = ftpl->namespace_;
    func->info.function.symbol = sym;
    SymbolSetCXXMangledAsmName(sym);

    Symbol* in_scope = SyntaxRegisterInstantiatedFriendFunction(
        parser->syntax, ftpl->namespace_, sym);
    StructAddFriendFunction(record_into, in_scope != NULL ? in_scope : sym);

    bool is_new_symbol = (in_scope == sym);
    if (is_new_symbol && ftpl->type->info.function.body != NULL &&
        constraints_satisfied &&
        !PendingTemplateInstantiationHasAsmName(sym->asm_name.value)) {
      sym->type->info.function.body = CloneTemplateFunctionBody(
          parser, ftpl->type, sym->type, args);
      sym->type->info.function.definition = true;
      sym->flags.is_defined = true;
      if (sym->type->info.function.is_inline) {
        sym->flags.is_inline_defn = true;
        if (!StorageIs(sym->storage, STO(static))) {
          sym->flags.is_weak = true;
        }
      }
      Vector* declarations = NewVector();
      VectorAppend(declarations,
                   NewVariableDeclarationASTNode(sym, NULL, sym->location));
      VectorAppend(&compiler->pending_template_instantiations,
                   NewDeclarationListASTNode(declarations, sym->location));
      VectorAppend(&compiler->declaration_asts,
                   sym->type->info.function.body);
    } else if (is_new_symbol && ftpl->type->info.function.body != NULL) {
      sym->value.func_defn = ftpl;
    }

    parser->template_substitution_source = saved_source;
    parser->template_substitution_target = saved_target;
  }
}

static void InstantiateTemplateFriendFunctions(TypeParser* parser, Struct* str,
                                               Struct* source_struct,
                                               Vector* args) {
  InstantiateTemplateFriendFunctionsImpl(parser, str, source_struct,
                                         source_struct, str, args);
}

/* Instantiate a class template `templ` with arguments `args`, returning the
 * concrete struct/union type (memoized by instantiation name so each unique
 * argument set is built once). Steps: handle alias templates; complete default
 * arguments; reuse an existing instantiation tag if present; select the best
 * partial specialization; create the instantiated struct tag; then instantiate
 * bases and members (expanding base/member packs) and lay out the struct. */
TypeRecord* InstantiateSimpleClassTemplate(TypeParser* parser,
                                                  Symbol* templ,
                                                  Vector* args) {
  return InstantiateSimpleClassTemplateImpl(parser, templ, args,
                                            /*emit_constraint_error=*/true);
}

static TypeRecord* InstantiateSimpleClassTemplateImpl(
    TypeParser* parser, Symbol* templ, Vector* args,
    bool emit_constraint_error) {
  TypeRecord* alias_type =
      InstantiateAliasClassTemplateImpl(parser, templ, args,
                                        emit_constraint_error);
  if (alias_type != NULL) {
    return alias_type;
  }
  if (templ == NULL || templ->type == NULL) {
    return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }
  alias_type = InstantiateGenericAliasTemplate(parser, templ, args,
                                               emit_constraint_error);
  if (alias_type != NULL) {
    return alias_type;
  }
  if (!TypeIsStructOrUnion(templ->type) ||
      templ->type->info.struct_info == NULL ||
      !templ->type->info.struct_info->is_template) {
    return TypeRecordCopy(templ->type);
  }
  Struct* template_struct = templ->type->info.struct_info;
  Vector* completed_args =
      CompleteClassTemplateArguments(parser, template_struct, args);
  if (completed_args == NULL) {
    return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }

  Vector* partial_args = NULL;
  ClassTemplatePartialSpecialization* partial =
      SelectClassTemplatePartialSpecialization(parser, templ, completed_args,
                                               &partial_args);
  ConstraintExpr* active_constraint = template_struct->associated_constraint;
  Vector* constraint_args = completed_args;
  if (partial != NULL) {
    active_constraint = partial->associated_constraint;
    constraint_args = partial_args;
  }
  if (!ConceptsConstraintSatisfied(active_constraint, constraint_args)) {
    TypeRecord* failure = ClassTemplateConstraintFailureType(
        parser, templ, active_constraint, constraint_args,
        emit_constraint_error);
    if (partial_args != NULL) {
      VectorDeleteWithContents(partial_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return failure;
  }

  String instantiated_name;
  StringInit(&instantiated_name, NULL);
  AppendTemplateInstantiationName(&instantiated_name, templ, completed_args);
  Symbol* existing =
      FindTemplateInstantiationTag(parser, templ, &instantiated_name);
  if (existing != NULL && existing->type != NULL &&
      TypeIsStructOrUnion(existing->type) &&
      existing->type->info.struct_info != NULL &&
      !existing->type->info.struct_info->is_template) {
    StringDestruct(&instantiated_name);
    if (partial_args != NULL) {
      VectorDeleteWithContents(partial_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return TypeRecordCopy(existing->type);
  }
  if (existing != NULL) {
    SyntaxError(parser->syntax,
                "Class template instantiation conflicts with existing tag %s",
                instantiated_name.value);
    StringDestruct(&instantiated_name);
    if (partial_args != NULL) {
      VectorDeleteWithContents(partial_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return TypeRecordCopy(templ->type);
  }
  Struct* source_struct = template_struct;
  Vector* source_args = completed_args;
  if (partial != NULL && partial->tag_symbol != NULL &&
      partial->tag_symbol->type != NULL &&
      TypeIsStructOrUnion(partial->tag_symbol->type) &&
      partial->tag_symbol->type->info.struct_info != NULL) {
    source_struct = partial->tag_symbol->type->info.struct_info;
    source_args = partial_args;
  }

  // When the primary template has only been forward-declared (its body not yet
  // parsed) and it has no partial specializations, do not materialize -- and
  // above all do not *cache* -- a concrete specialization now.  The primary's
  // member set is empty, so caching it would permanently shadow the real
  // specialization once the definition is seen.  This arises when another
  // template's member signature names a class template that is completed later
  // in the TU (e.g. std::basic_string's conversion to the still-forward-
  // declared std::basic_string_view).  Return the deferred template-id
  // representation (a copy of the primary template type carrying the concrete
  // arguments) exactly as SubstituteTemplateIdType does for still-dependent
  // arguments; TypeMaterializeClassTemplateSpecialization then re-instantiates
  // it against the completed definition once the type is required to be
  // complete.
  //
  // The no-partial-specialization guard is essential: a deliberately
  // incomplete traits primary (e.g. std::tuple_size, declared once and only
  // ever completed through partial/explicit specializations) must still
  // instantiate to its empty primary here for arguments that match no
  // specialization -- deferring it would leave consumers that immediately need
  // the (empty) type, such as the structured-bindings `tuple_size<E>::value`
  // lookup, unable to complete it.
  if (partial == NULL && template_struct->tag_symbol != NULL &&
      !template_struct->tag_symbol->flags.is_defined &&
      template_struct->partial_specializations.length == 0) {
    StringDestruct(&instantiated_name);
    TypeRecord* deferred = TypeRecordCopy(templ->type);
    if (deferred->template_arguments != NULL) {
      VectorDeleteWithContents(deferred->template_arguments,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    deferred->template_origin = templ;
    deferred->template_arguments = TemplateArgumentVectorCopy(completed_args);
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return deferred;
  }

  if (!ClassTemplateInstantiationMembersSupported(parser, source_struct)) {
    StringDestruct(&instantiated_name);
    if (partial_args != NULL) {
      VectorDeleteWithContents(partial_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return TypeRecordCopy(templ->type);
  }

  Struct* str = NewStruct(source_struct->is_union);
  str->is_class = source_struct->is_class;
  str->lexical_parent = source_struct->lexical_parent;
  str->packed = source_struct->packed;
  str->explicit_alignment = source_struct->explicit_alignment;
  str->pack = source_struct->pack;
  // Carry friend classes from the template to each instantiation verbatim;
  // friend *functions* are instantiated per specialization below, after the
  // members are in place, so their dependent signatures and inline bodies can
  // be substituted with the template arguments.
  for (size_t i = 0; i < source_struct->friend_classes.length; i++) {
    StructAddFriendClass(str, source_struct->friend_classes.value.p[i]);
  }
  TypeRecord* type = NewTypeRecord(source_struct->is_union ? kTypeUnion
                                                           : kTypeStruct,
                                  kQualPlain);
  type->info.struct_info = str;
  type->template_origin = templ;
  type->template_arguments = TemplateArgumentVectorCopy(completed_args);
  Symbol* tag = NewSymbol(instantiated_name.value, type, STO(implicit));
  tag->flags.is_defined = true;
  str->tag_name = &tag->name;
  str->tag_symbol = tag;
  if (!AddTemplateInstantiationTag(parser, templ, tag)) {
    SyntaxError(parser->syntax,
                "Class template instantiation conflicts with existing tag %s",
                instantiated_name.value);
    StringDestruct(&instantiated_name);
    if (partial_args != NULL) {
      VectorDeleteWithContents(partial_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return TypeRecordCopy(templ->type);
  }

  Struct* saved_substitution_source = parser->template_substitution_source;
  Struct* saved_substitution_target = parser->template_substitution_target;
  parser->template_substitution_source = source_struct;
  parser->template_substitution_target = str;

  for (size_t i = 0; i < source_struct->bases.length; i++) {
    CXXBaseSpecifier* template_base = source_struct->bases.value.p[i];
    int pack_index = -1;
    if (template_base->is_pack_expansion &&
        TypeIsTemplateParameterPlaceholder(template_base->type, &pack_index) &&
        pack_index >= 0 && (size_t)pack_index < source_args->length) {
      TemplateArgument* pack = source_args->value.p[pack_index];
      if (pack != NULL && pack->pack_arguments != NULL) {
        for (size_t j = 0; j < pack->pack_arguments->length; j++) {
          TemplateArgument* element = pack->pack_arguments->value.p[j];
          if (element == NULL || element->kind != kTemplateParameterType ||
              element->type == NULL || !TypeIsStructOrUnion(element->type)) {
            SyntaxError(parser->syntax,
                        "Base class pack expansion requires class types");
            continue;
          }
          TypeRecord* base_type = TypeRecordCopy(element->type);
          TypeRecordCalculateSize(base_type);
          VectorAppend(&str->bases,
                       NewCXXBaseSpecifier(base_type, template_base->access,
                                           template_base->is_virtual));
          TypeRecordDelete(base_type);
        }
        continue;
      }
    }
    TypeRecord* base_type =
        SubstituteTemplateParameters(parser, template_base->type,
                                     source_args);
    base_type = TypeMaterializeClassTemplateSpecialization(parser->syntax,
                                                           base_type);
    if (!TypeIsStructOrUnion(base_type)) {
      SyntaxError(parser->syntax, "base class must be a class or struct type");
      TypeRecordDelete(base_type);
      continue;
    }
    TypeRecordCalculateSize(base_type);
    VectorAppend(&str->bases,
                 NewCXXBaseSpecifier(base_type, template_base->access,
                                     template_base->is_virtual));
    TypeRecordDelete(base_type);
  }
  CollectCXXVirtualBases(str);
  CopyCXXBaseVirtualMembers(str);
  LayoutCXXBaseSpecifiers(str);
  ApplyCXXMemberUsingDeclarations(parser, str, source_struct, source_args);

  // Member function bodies are cloned in a second pass, after every member
  // function signature has been added to `str`, so that a member's body may
  // reference other members declared later in the class (e.g. `operator=`
  // calling a later-declared `emplace`).
  Vector pending_member_bodies;
  VectorInit(&pending_member_bodies);
  // Nested classes whose deferred hidden friends must be materialized once the
  // enclosing instantiation is complete (parallel source/target vectors).
  Vector pending_nested_friend_sources;
  Vector pending_nested_friend_targets;
  VectorInit(&pending_nested_friend_sources);
  VectorInit(&pending_nested_friend_targets);
  for (size_t i = 0; i < source_struct->members.length; i++) {
    StructMember* member = source_struct->members.value.p[i];
    if (StructMemberIsNestedType(member)) {
      TypeRecord* nested_type =
          SubstituteTemplateParameters(parser, member->symbol->type,
                                       source_args);
      // A member type alias may substitute to a lazy dependent-member type such
      // as `typename ratio<1,1000>::type` (from `using period = typename
      // Period::type;`).  Now that the arguments are concrete, collapse it to
      // the real class specialization so later qualified accesses through this
      // alias (e.g. `To::period::num`) resolve to the specialization's members
      // and fold, instead of emitting an unresolved bare `num`/`den` symbol.
      nested_type =
          TypeMaterializeClassTemplateSpecialization(parser->syntax, nested_type);
      if (TypeIsStructOrUnion(nested_type) &&
          nested_type->info.struct_info != NULL &&
          member->symbol->type != NULL &&
          TypeIsStructOrUnion(member->symbol->type) &&
          member->symbol->type->info.struct_info != NULL &&
          member->symbol->type->info.struct_info->lexical_parent ==
              source_struct) {
        Struct* nested_source = member->symbol->type->info.struct_info;
        Struct* nested_target = nested_type->info.struct_info;
        nested_target->lexical_parent = str;
        for (size_t j = 0; j < nested_target->members.length; j++) {
          StructMember* nested_func_member = nested_target->members.value.p[j];
          for (StructMember* overload = nested_func_member; overload != NULL;
               overload = overload->overload_next) {
            if (overload->is_member_function && overload->symbol != NULL) {
              SymbolSetCXXMangledAsmName(overload->symbol);
            }
          }
        }
        // A nested class (e.g. a view's __iterator/__sentinel) may declare
        // hidden-friend operators.  Defer materializing them until the whole
        // enclosing class is formed (below), so a sentinel's friend that names
        // the iterator type resolves against the already-instantiated sibling.
        VectorAppend(&pending_nested_friend_sources, nested_source);
        VectorAppend(&pending_nested_friend_targets, nested_target);
      }
      Symbol* nested_symbol =
          NewSymbol(member->symbol->name.value, nested_type, STO(typedef));
      nested_symbol->flags = member->symbol->flags;
      nested_symbol->location = member->symbol->location;
      StructMember* nested_member = NewStructMember(nested_symbol);
      nested_member->access = member->access;
      AddStructMember(parser, str, nested_member);
      continue;
    }
    if (member->is_member_function) {
      StructMember* instantiated = InstantiateTemplateMemberFunction(
          parser, str, member, source_args, &pending_member_bodies);
      StructMember* existing =
          MapFindPointerKey(&str->symbol_table, &instantiated->symbol->name);
      if (existing != NULL) {
        AppendStructMemberOverload(parser, str, existing, instantiated);
      } else {
        AddStructMember(parser, str, instantiated);
      }
      continue;
    }
    TypeRecord* member_type =
        SubstituteTemplateParameters(parser, member->symbol->type,
                                     source_args);
    TypeRecordCalculateSize(member_type);
    Symbol* member_symbol =
        NewSymbol(member->symbol->name.value, member_type, member->symbol->storage);
    member_symbol->location = member->symbol->location;
    member_symbol->flags = member->symbol->flags;
    member_symbol->value = member->symbol->value;
    member_symbol->dependent_value_template_parameter_index =
        member->symbol->dependent_value_template_parameter_index;
    SubstituteDependentSymbolValue(member_symbol, source_args);
    SubstituteStaticMemberInitializerValue(parser, member_symbol,
                                           member->default_initializer,
                                           source_args);
    StructMember* instantiated = NewStructMember(member_symbol);
    // Substitute template parameters in a non-static default member initializer
    // (e.g. `W value = W();`).  A plain clone would leave the parameter-typed
    // value-initialization `W()` referencing the template parameter, which then
    // lowers to an undefined symbol; substitution rewrites it to e.g. `int()`.
    if (member->default_initializer != NULL && !member->is_static) {
      ASTNode* substituted = CloneDependentExpressionWithArgs(
          parser, member->default_initializer, source_args);
      instantiated->default_initializer =
          substituted != NULL
              ? substituted
              : CloneCXXDefaultMemberInitializer(member->default_initializer);
    } else {
      instantiated->default_initializer =
          CloneCXXDefaultMemberInitializer(member->default_initializer);
    }
    instantiated->access = member->access;
    instantiated->is_anon = member->is_anon;
    instantiated->is_static = member->is_static;
    instantiated->is_mutable = member->is_mutable;
    instantiated->is_member_function = member->is_member_function;
    instantiated->is_using_declaration = member->is_using_declaration;
    instantiated->bit_size = member->bit_size;
    instantiated->bit_offset = member->bit_offset;
    instantiated->cxx_vcall_offset = member->cxx_vcall_offset;
    if (!instantiated->is_static && !instantiated->is_using_declaration &&
        !StructMemberIsNestedType(instantiated)) {
      AlignNextOffset(str, member_type);
      instantiated->byte_offset = str->next_offset;
    } else {
      instantiated->byte_offset = member->byte_offset;
    }
    instantiated->index = str->members.length;
    AddStructMember(parser, str, instantiated);
    if (!instantiated->is_static && !instantiated->is_using_declaration &&
        !StructMemberIsNestedType(instantiated)) {
      UpdateStructSize(str, member_type, str->is_union);
    }
  }
  FinalizeStructAlignment(str);
  TypeRecordCalculateSize(type);
  ComputeCXXAggregateStatus(str);
  AddImplicitCXXSpecialMembers(parser, str, tag);
  AddImplicitCXXDestructorIfNeeded(parser, str, tag);
  AddImplicitCXXDeductionGuides(str, tag);
  // Complete the polymorphic layout of the instantiation exactly as a normal
  // class definition does (type_class.c): insert the hidden vptr/vbptr, lay out
  // virtual bases, and emit the vtable(s) and virtual-base table(s).  Without
  // this an instantiated class template with virtual members would carry a
  // virtual_members slot map but no vptr field and no emitted vtable, so its
  // constructors could not initialize the vptr and virtual calls would read
  // garbage.  This runs before the member-body clone pass below so that the
  // cloned constructor preambles observe vtables_registered and emit their vptr
  // initializers directly.
  UpdateCXXAbstractStatus(str);
  AddCXXVPtrMember(parser, str);
  AddCXXVBPtrMember(parser, str);
  str->non_virtual_size = str->next_offset;
  LayoutCXXVirtualBaseSpecifiers(str);
  RegisterCXXVTable(parser, str);
  RegisterCXXVBTables(parser, str);
  CXXFixupSpecialMemberTrivialityAfterLayout(str);
  str->vtables_registered = true;
  SyntaxFlushPendingVPtrInitializers(str);
  // The vptr/vbptr insertion above shifts member offsets and grows the object,
  // so re-finalize the alignment and recompute the cached type size.
  FinalizeStructAlignment(str);
  TypeRecordCalculateSize(type);
  // Second pass: with the class fully formed (all members, layout, and implicit
  // special members in place), clone the deferred member function bodies so a
  // body may reference any other member regardless of declaration order.
  for (size_t i = 0; i < pending_member_bodies.length; i++) {
    PendingMemberBody* pmb = pending_member_bodies.value.p[i];
    CloneInstantiatedMemberFunctionBody(parser, str, pmb->symbol,
                                        pmb->template_definition,
                                        pmb->substitution_source, source_args);
    free(pmb);
  }
  VectorDestruct(&pending_member_bodies);
  InstantiateTemplateFriendFunctions(parser, str, source_struct, source_args);
  // Materialize nested classes' hidden friends now that every nested type is a
  // member of `str`, so cross-references (e.g. sentinel friend naming iterator)
  // remap by name through the enclosing source->target mapping.
  for (size_t i = 0; i < pending_nested_friend_sources.length; i++) {
    Struct* nested_source = pending_nested_friend_sources.value.p[i];
    Struct* nested_target = pending_nested_friend_targets.value.p[i];
    InstantiateTemplateFriendFunctionsImpl(parser, nested_target, nested_source,
                                           source_struct, str, source_args);
  }
  VectorDestruct(&pending_nested_friend_sources);
  VectorDestruct(&pending_nested_friend_targets);
  parser->template_substitution_source = saved_substitution_source;
  parser->template_substitution_target = saved_substitution_target;
  StringDestruct(&instantiated_name);
  if (partial_args != NULL) {
    VectorDeleteWithContents(partial_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return TypeRecordCopy(type);
}

/* Public entry point: instantiate class template `templ` with `args`. */
TypeRecord* TypeInstantiateClassTemplate(Syntax* syntax, Symbol* templ,
                                         Vector* args) {
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  TypeRecord* type = InstantiateSimpleClassTemplateImpl(&parser, templ, args,
                                                        /*emit_constraint_error=*/true);
  TypeParserDestruct(&parser);
  return type;
}

/* Like TypeInstantiateClassTemplate, but rejects unsatisfied constraints
 * without emitting diagnostics (for speculative CTAD/deduction). */
TypeRecord* TypeInstantiateClassTemplateQuiet(Syntax* syntax, Symbol* templ,
                                              Vector* args) {
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  TypeRecord* type = InstantiateSimpleClassTemplateImpl(&parser, templ, args,
                                                        /*emit_constraint_error=*/false);
  TypeParserDestruct(&parser);
  return type;
}

/* See TypeMaterializeClassTemplateSpecialization.  Recurses through
 * pointer/reference spines so a capture field typed `variant<int,long>*` is
 * rewritten to point at the concrete specialization. */
TypeRecord* TypeMaterializeClassTemplateSpecialization(Syntax* syntax,
                                                       TypeRecord* type) {
  if (syntax == NULL || type == NULL || !CompilerIsCXX()) {
    return type;
  }
  if (TypeIsPointer(type) || TypeIsReference(type)) {
    TypeRecord* next =
        TypeMaterializeClassTemplateSpecialization(syntax, type->next);
    if (next == type->next) {
      return type;
    }
    TypeRecord* copy = TypeRecordCopy(type);
    TypeRecordIncRef(next);
    TypeRecordDelete(copy->next);
    copy->next = next;
    copy->type = next != NULL ? next->type : copy->type;
    return TypeRecordCalculateSize(copy);
  }
  if (type->template_origin != NULL && type->template_arguments != NULL &&
      type->dependent_member_name != NULL &&
      !TemplateArgumentVectorContainsTemplateParameter(type->template_arguments)) {
    TypeRecord* owner = TypeInstantiateClassTemplate(
        syntax, type->template_origin, type->template_arguments);
    if (owner != NULL && TypeIsStructOrUnion(owner) &&
        owner->info.struct_info != NULL) {
      StructMember* member =
          FindStructMember(owner->info.struct_info, type->dependent_member_name);
      if (member != NULL && member->symbol != NULL &&
          StorageIs(member->symbol->storage, STO(typedef))) {
        TypeRecord* resolved = TypeRecordCopy(member->symbol->type);
        resolved->qualifiers |= type->qualifiers;
        TypeRecordDelete(owner);
        return TypeRecordCalculateSize(resolved);
      }
    }
    TypeRecordDelete(owner);
  }
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return type;
  }
  Struct* str = type->info.struct_info;
  // Concrete specializations are not flagged as templates; only the primary
  // (still carrying unresolved-looking template_arguments) needs materializing.
  if (!str->is_template || str->tag_symbol == NULL) {
    return type;
  }
  Symbol* origin = type->template_origin;
  Vector* args = type->template_arguments;
  if (origin == NULL && str->tag_symbol->type != NULL) {
    origin = str->tag_symbol->type->template_origin;
    if (args == NULL) {
      args = str->tag_symbol->type->template_arguments;
    }
  }
  if (origin == NULL && str->tag_symbol->flags.is_template) {
    origin = str->tag_symbol;
  }
  if (origin == NULL || args == NULL ||
      TemplateArgumentVectorContainsTemplateParameter(args)) {
    return type;
  }
  TypeRecord* concrete = TypeInstantiateClassTemplate(syntax, origin, args);
  if (concrete == NULL) {
    return type;
  }
  concrete->qualifiers |= type->qualifiers;
  return concrete;
}

bool SymbolIsInStdNamespace(Symbol* symbol) {
  return symbol != NULL && symbol->namespace_ != NULL &&
         symbol->namespace_->qualified_name.value != NULL &&
         strcmp(symbol->namespace_->qualified_name.value, "std") == 0;
}

static bool CXXSymbolIsStdInitializerListTemplate(Symbol* symbol) {
  return symbol != NULL && strcmp(symbol->name.value, "initializer_list") == 0 &&
         SymbolIsInStdNamespace(symbol);
}

static bool CXXSymbolNamesStdInitializerList(Symbol* symbol) {
  if (symbol == NULL) {
    return false;
  }
  const char* name = symbol->name.value;
  if (name == NULL ||
      (strcmp(name, "initializer_list") != 0 &&
       strncmp(name, "initializer_list<", 17) != 0)) {
    return false;
  }
  return symbol->namespace_ == NULL ||
         strcmp(symbol->namespace_->qualified_name.value, "std") == 0;
}

bool TypeIsCXXInitializerList(TypeRecord* type) {
  if (!CompilerIsCXX() || type == NULL) {
    return false;
  }
  if (CXXSymbolIsStdInitializerListTemplate(type->template_origin)) {
    return true;
  }
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL ||
      type->info.struct_info->tag_symbol == NULL) {
    return false;
  }
  Symbol* tag = type->info.struct_info->tag_symbol;
  if (CXXSymbolNamesStdInitializerList(tag)) {
    return true;
  }
  if (CXXSymbolIsStdInitializerListTemplate(tag->type != NULL
                                                ? tag->type->template_origin
                                                : NULL)) {
    return true;
  }
  return CXXSymbolIsStdInitializerListTemplate(type->template_origin);
}

/* Public: the element type T of a `std::initializer_list<T>` type, or NULL. */
TypeRecord* TypeCXXInitializerListElement(TypeRecord* type) {
  if (!TypeIsCXXInitializerList(type) || type->template_arguments == NULL ||
      type->template_arguments->length != 1) {
    return NULL;
  }
  TemplateArgument* arg = type->template_arguments->value.p[0];
  return arg != NULL && arg->kind == kTemplateParameterType ? arg->type : NULL;
}

/* Public: instantiate `std::initializer_list<element_type>` (used for braced
 * initializer expressions), looking up the std template. Returns NULL if the
 * std type is unavailable. */
TypeRecord* TypeInstantiateCXXInitializerList(Syntax* syntax,
                                              TypeRecord* element_type) {
  if (!CompilerIsCXX() || syntax == NULL || element_type == NULL) {
    return NULL;
  }
  Namespace* std_ns = NamespaceFindStdNamespace();
  if (std_ns == NULL) {
    return NULL;
  }
  String initializer_list_name;
  StringInit(&initializer_list_name, "initializer_list");
  NamespaceInlineSymbolLookup result =
      NamespaceResolveSymbolInInlineSet(std_ns, &initializer_list_name);
  Symbol* templ = result.status == kInlineLookupUnique ? result.symbol : NULL;
  StringDestruct(&initializer_list_name);
  if (!CXXSymbolIsStdInitializerListTemplate(templ) || !templ->flags.is_template) {
    return NULL;
  }
  Vector* args = NewVector();
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  memset(arg, 0, sizeof(*arg));
  arg->kind = kTemplateParameterType;
  arg->is_pack_expansion = false;
  arg->type = TypeRecordCopy(element_type);
  arg->int_value = 0;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NULL;
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  VectorAppend(args, arg);
  TypeRecord* type = TypeInstantiateClassTemplate(syntax, templ, args);
  VectorDeleteWithContents(args, (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return type;
}

/* Public: look up a C++20 comparison-category type (e.g. "strong_ordering")
 * declared in namespace std.  Returns the struct TypeRecord, or NULL if the
 * type is not visible (typically because <compare> was not included). */
TypeRecord* TypeFindCXXComparisonCategory(const char* category_name) {
  if (!CompilerIsCXX() || compiler == NULL || category_name == NULL) {
    return NULL;
  }
  Namespace* std_ns = NamespaceFindStdNamespace();
  if (std_ns == NULL) {
    return NULL;
  }
  String name;
  StringInit(&name, category_name);
  NamespaceInlineTagLookup result = NamespaceResolveTagInInlineSet(std_ns, &name);
  Symbol* tag = result.status == kInlineLookupUnique ? result.tag : NULL;
  StringDestruct(&name);
  if (tag == NULL || tag->type == NULL || !TypeIsStructOrUnion(tag->type)) {
    return NULL;
  }
  return tag->type;
}

/* Public entry point: instantiate function template `templ` with `args`. */
Symbol* TypeInstantiateFunctionTemplate(Syntax* syntax, Symbol* templ,
                                        Vector* args) {
  if (!ConceptsFunctionTemplateConstraintsSatisfied(templ, args)) {
    return templ;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  Symbol* symbol = InstantiateSimpleFunctionTemplate(&parser, templ, args);
  TypeParserDestruct(&parser);
  return symbol;
}

/* True if `alias` is an alias template whose right-hand side is itself a class
 * template specialization (e.g. `using X = vector<T>;`), as opposed to a plain
 * type alias. Such aliases participate in class-template instantiation/CTAD. */
bool CXXAliasTemplatePatternNamesClassTemplate(Symbol* alias) {
  if (!CompilerIsCXX() || alias == NULL || !alias->flags.is_template ||
      !StorageIs(alias->storage, STO(typedef)) || alias->type == NULL ||
      alias->type->dependent_member_name != NULL ||
      !TypeIsStructOrUnion(alias->type) ||
      alias->type->template_origin == NULL ||
      alias->type->template_arguments == NULL ||
      alias->type->template_origin->type == NULL ||
      !TypeIsStructOrUnion(alias->type->template_origin->type) ||
      alias->type->template_origin->type->info.struct_info == NULL ||
      !alias->type->template_origin->type->info.struct_info->is_template) {
    return false;
  }
  size_t expected =
      (size_t)alias->type->template_origin->type->info.struct_info
          ->template_parameter_count;
  if (alias->type->template_arguments->length != expected) {
    return false;
  }
  return TemplateArgumentVectorContainsTemplateParameter(
      alias->type->template_arguments);
}

/* Mark `type` as a CTAD placeholder for an alias template: point it at the
 * alias's underlying class template and copy the alias's argument pattern, so
 * deduction runs against the alias rather than the underlying template. */
void SetCXXAliasTemplatePlaceholderOrigin(Symbol* alias,
                                                 TypeRecord* type) {
  if (!CXXAliasTemplatePatternNamesClassTemplate(alias) || type == NULL) {
    return;
  }
  if (type->template_arguments != NULL) {
    VectorDeleteWithContents(type->template_arguments,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  type->template_arguments = TemplateArgumentVectorCopy(alias->type->template_arguments);
  type->template_origin = alias;
  type->info.struct_info = alias->type->template_origin->type->info.struct_info;
}

/* True if an alias template argument is a pack expansion (`Ts...`), reporting
 * the referenced pack parameter index and its kind. */
bool AliasTemplateArgumentIsPackExpansion(TemplateArgument* arg,
                                                 int* pack_index,
                                                 TemplateParameterKind* kind) {
  if (arg == NULL || !arg->is_pack_expansion) {
    return false;
  }
  if (arg->kind == kTemplateParameterType) {
    int index = -1;
    if (TypeIsTemplateParameterPlaceholder(arg->type, &index) && index >= 0) {
      if (pack_index != NULL) {
        *pack_index = index;
      }
      if (kind != NULL) {
        *kind = kTemplateParameterType;
      }
      return true;
    }
  }
  if (arg->kind == kTemplateParameterNonType &&
      arg->template_parameter_index >= 0) {
    if (pack_index != NULL) {
      *pack_index = arg->template_parameter_index;
    }
    if (kind != NULL) {
      *kind = kTemplateParameterNonType;
    }
    return true;
  }
  if (arg->kind == kTemplateParameterTemplate &&
      arg->template_parameter_index >= 0) {
    if (pack_index != NULL) {
      *pack_index = arg->template_parameter_index;
    }
    if (kind != NULL) {
      *kind = kTemplateParameterTemplate;
    }
    return true;
  }
  return false;
}

/* Fill in an alias template's argument list from the supplied `actuals`,
 * applying defaults and gathering a trailing parameter pack, so the count
 * matches the alias's parameters. Returns NULL on an arity mismatch. */
TypeRecord* InstantiateAliasClassTemplate(TypeParser* parser,
                                                 Symbol* alias,
                                                 Vector* args) {
  return InstantiateAliasClassTemplateImpl(parser, alias, args,
                                           /*emit_constraint_error=*/true);
}

static TypeRecord* InstantiateAliasClassTemplateImpl(TypeParser* parser,
                                                     Symbol* alias,
                                                     Vector* args,
                                                     bool emit_constraint_error) {
  if (!CXXAliasTemplatePatternNamesClassTemplate(alias) ||
      alias->type->template_origin == NULL ||
      alias->type->template_arguments == NULL) {
    return NULL;
  }
  Vector* completed_alias_args = CompleteAliasTemplateArguments(alias, args);
  if (completed_alias_args == NULL) {
    return NULL;
  }
  if (!ConceptsConstraintSatisfied(alias->associated_constraint,
                                   completed_alias_args)) {
    if (emit_constraint_error) {
      ReportAliasTemplateConstraintFailure(parser, alias, completed_alias_args);
    }
    VectorDeleteWithContents(completed_alias_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }
  Vector* underlying_args =
      SubstituteTemplateArgumentVectorForTypes(parser,
                                              alias->type->template_arguments,
                                              completed_alias_args);
  TypeRecord* instantiated = InstantiateSimpleClassTemplateImpl(
      parser, alias->type->template_origin, underlying_args,
      emit_constraint_error);
  VectorDeleteWithContents(underlying_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  VectorDeleteWithContents(completed_alias_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return instantiated;
}

/* Expand an alias template whose pattern is not a bare class-template
 * template-id (the shape `InstantiateAliasClassTemplateImpl` handles), for
 * example `template <class R> using all_t = decltype(views::all(declval<R>()))`
 * whose pattern is an already-formed specialization such as `ref_view<R>`.
 *
 * The pattern must be substituted with the alias's own arguments.  Returning it
 * unsubstituted leaves the alias's parameter indices in the result, and those
 * indices then bind to whatever occupies the same index in the enclosing
 * template -- so inside a member function template of a class template, the
 * alias parameter silently resolves to the class's argument.
 *
 * Returns NULL when `templ` is not an alias template, leaving the caller's
 * class-template path in charge. */
static TypeRecord* InstantiateGenericAliasTemplate(TypeParser* parser,
                                                   Symbol* alias, Vector* args,
                                                   bool emit_constraint_error) {
  if (!CompilerIsCXX() || alias == NULL || !alias->flags.is_template ||
      !StorageIs(alias->storage, STO(typedef)) || alias->type == NULL) {
    return NULL;
  }
  // A struct-typed pattern that is still the *primary* class template belongs to
  // the ordinary class-template path, not here.
  if (TypeIsStructOrUnion(alias->type) &&
      alias->type->info.struct_info != NULL &&
      alias->type->info.struct_info->is_template) {
    return NULL;
  }
  Vector* completed_args = CompleteAliasTemplateArguments(alias, args);
  if (completed_args == NULL) {
    return NULL;
  }
  if (!ConceptsConstraintSatisfied(alias->associated_constraint,
                                   completed_args)) {
    if (emit_constraint_error) {
      ReportAliasTemplateConstraintFailure(parser, alias, completed_args);
    }
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }
  TypeRecord* subst =
      SubstituteTemplateParameters(parser, alias->type, completed_args);
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (subst == NULL) {
    return NULL;
  }
  return TypeRecordCalculateSize(subst);
}

/* Public: build a CTAD placeholder type from a class-template (or alias
 * template) symbol, used when a variable is declared with just the template
 * name (`Foo x = ...;`). Returns NULL if `symbol` is not a class template. */
TypeRecord* TypeClassTemplatePlaceholderFromSymbol(Symbol* symbol) {
  if (!CompilerIsCXX() || symbol == NULL || !symbol->flags.is_template ||
      symbol->type == NULL || !TypeIsStructOrUnion(symbol->type)) {
    return NULL;
  }
  TypeRecord* type = TypeRecordCopy(symbol->type);
  if (CXXAliasTemplatePatternNamesClassTemplate(symbol)) {
    SetCXXAliasTemplatePlaceholderOrigin(symbol, type);
  } else if (type->info.struct_info != NULL &&
             type->info.struct_info->is_template) {
    type->template_origin = symbol;
  }
  if (!TypeIsClassTemplatePlaceholder(type)) {
    TypeRecordDelete(type);
    return NULL;
  }
  return type;
}
