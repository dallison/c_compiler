//
//  symbol.c
//  c_compiler
//
//  Created by David Allison on 10/28/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "symbol.h"
#include "concepts.h"
#include "type_internal.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "dstring.h"
#include "compiler.h"
#include "member_pointer.h"
#include "module_identity.h"
#include "symbol_table.h"
#include "type_template_internal.h"

bool StorageIs(Storage storage, Storage value) {
  return (storage & value) != 0;
}

// Normalizes an attribute name by stripping a surrounding "__" pair, so that
// "__packed__" is treated the same as "packed" (as GCC does).
static void NormalizeAttributeName(String* name) {
  size_t len = name->length;
  if (len >= 5 && name->value[0] == '_' && name->value[1] == '_' &&
      name->value[len - 1] == '_' && name->value[len - 2] == '_') {
    String stripped = {0};
    StringInitFromSegment(&stripped, name->value + 2, len - 4);
    StringSetString(name, &stripped);
    StringDestruct(&stripped);
  }
}

Attribute* NewAttribute(const char* name) {
  Attribute* attr = malloc(sizeof(Attribute));
  StringInit(&attr->name, name);
  NormalizeAttributeName(&attr->name);
  VectorInit(&attr->args);
  attr->dependent_alignas_type = NULL;
  attr->dependent_alignas_expr = NULL;
  attr->annotation_expr = NULL;
  attr->annotation_value = NULL;
  return attr;
}

void AttributeDestruct(Attribute* attr) {
  StringDestruct(&attr->name);
  VectorDestructWithContents(&attr->args, (VectorElementDestructor)StringDestruct,
                             /*free_element=*/true);
  TypeRecordDelete(attr->dependent_alignas_type);
  ASTNodeDelete(attr->dependent_alignas_expr);
  ASTNodeDelete(attr->annotation_expr);
}

void AttributeDelete(Attribute* attr) {
  AttributeDestruct(attr);
  free(attr);
}

void AttributeAddArg(Attribute* attr, const char* arg, size_t length) {
  VectorAppend(&attr->args, NewStringWithLength(arg, length));
}

Attribute* AttributeClone(Attribute* attr) {
  Attribute* copy = malloc(sizeof(Attribute));
  StringInit(&copy->name, attr->name.value);
  VectorInit(&copy->args);
  for (size_t i = 0; i < attr->args.length; i++) {
    String* a = attr->args.value.p[i];
    VectorAppend(&copy->args, NewString(a->value));
  }
  copy->dependent_alignas_type =
      attr->dependent_alignas_type != NULL
          ? TypeRecordCopy(attr->dependent_alignas_type)
          : NULL;
  copy->dependent_alignas_expr =
      ASTNodeClone(attr->dependent_alignas_expr, IdentityCloneNode,
                   NULL, NULL);
  copy->annotation_expr =
      ASTNodeClone(attr->annotation_expr, IdentityCloneNode, NULL, NULL);
  copy->annotation_value = attr->annotation_value;
  return copy;
}

size_t AttributeArgCount(Attribute* attr) { return attr->args.length; }

const char* AttributeArgString(Attribute* attr, size_t index) {
  if (index >= attr->args.length) {
    return NULL;
  }
  return ((String*)attr->args.value.p[index])->value;
}

bool AttributeArgInt(Attribute* attr, size_t index, long* value) {
  const char* s = AttributeArgString(attr, index);
  if (s == NULL || *s == '\0') {
    return false;
  }
  char* end = NULL;
  long v = strtol(s, &end, 10);
  if (end == s || *end != '\0') {
    return false;
  }
  *value = v;
  return true;
}

Attribute* AttributeListFind(Vector* attrs, const char* name) {
  for (size_t i = 0; i < attrs->length; i++) {
    Attribute* attr = attrs->value.p[i];
    if (StringEqual(&attr->name, name)) {
      return attr;
    }
  }
  return NULL;
}

bool AttributeListHas(Vector* attrs, const char* name) {
  return AttributeListFind(attrs, name) != NULL;
}

void AttributeListDestruct(Vector* attrs) {
  VectorDestructWithContents(attrs, (VectorElementDestructor)AttributeDestruct,
                             /*free_element=*/true);
}

void AttributeListClone(Vector* dest, Vector* src) {
  VectorInit(dest);
  for (size_t i = 0; i < src->length; i++) {
    VectorAppend(dest, AttributeClone((Attribute*)src->value.p[i]));
  }
}

void SymbolInit(Symbol* sym, const char* name, struct TypeRecord* type,
                Storage storage) {
  StringInit(&sym->name, name);
  StringInit(&sym->asm_name, NULL);
  sym->namespace_ = NULL;
  sym->type = NULL;
  sym->storage = storage;
  sym->flags.is_defined = false;
  sym->flags.is_tentative_decl = false;
  sym->flags.is_forward_declared = false;
  sym->flags.is_local = false;
  sym->flags.is_block_scope = false;
  sym->flags.is_argument = false;
  sym->flags.is_temp = false;
  sym->flags.address_taken = false;
  sym->flags.used = false;
  sym->flags.invented = false;
  sym->flags.is_inline_defn = false;
  sym->flags.value_set = false;
  sym->flags.noreturn = false;
  sym->flags.always_inline = false;
  sym->flags.noinline = false;
  sym->flags.is_using_alias = false;
  sym->flags.is_overloaded = false;
  sym->flags.is_template = false;
  sym->flags.is_template_parameter = false;
  sym->flags.is_template_type_parameter = false;
  sym->flags.is_template_template_parameter = false;
  sym->flags.is_parameter_pack = false;
  sym->flags.is_constexpr = false;
  sym->flags.is_constinit = false;
  sym->flags.is_weak = false;
  sym->flags.is_c_linkage = false;
  sym->flags.is_exported = false;
  sym->flags.is_concept = false;
  sym->flags.is_module_private = false;
  sym->flags.is_explicit_specialization = false;
  sym->flags.is_name_independent = false;
  sym->flags.name_independent_lookup_ambiguous = false;
  sym->concept_definition = NULL;
  sym->value.fvalue = 0;
  sym->stack_offset = 0;
  sym->alias_target = NULL;
  sym->lambda_capture_source = NULL;
  sym->lambda_capture_by_reference = false;
  sym->overload_next = NULL;
  sym->default_argument = NULL;
  sym->constexpr_initializer = NULL;
  sym->is_constexpr_representable = false;
  sym->requires_ast_constexpr = false;
  sym->constexpr_reference_scope = NULL;
  sym->variable_template = NULL;
  sym->alias_template = NULL;
  sym->template_template_parameters = NULL;
  sym->associated_constraint = NULL;
  sym->cxx_linkage = kCXXLinkageExternal;
  StringInit(&sym->owning_module_name, NULL);
  StringInit(&sym->owning_module_partition, NULL);
  StringInit(&sym->import_source_module, NULL);
  sym->location = 0;
  sym->usage_info.reads = 0;
  sym->usage_info.used_as_arg = 0;
  sym->usage_info.used_in_loop = 0;
  sym->id = compiler->next_symbol_id++;
  VectorInit(&sym->attributes);
  sym->alignment = 0;
  sym->template_parameter_index = -1;
  sym->dependent_value_template_parameter_index = -1;
  SymbolSetType(sym, type);
  sym->die = NULL;
  sym->is_imported_module_symbol = false;
  sym->destruction_complete = false;
  sym->is_read = false;
  sym->structured_binding_pack_size = -2;
  sym->is_nrvo = false;
  VectorInit(&sym->imported_function_template_parameters_backup);
  sym->cached_target_symbol_name = NULL;
}

Symbol* NewSymbol(const char* name, struct TypeRecord* type, Storage storage) {
  Symbol* sym = malloc(sizeof(Symbol));
  SymbolInit(sym, name, type, storage);
  return sym;
}

void SymbolBackupImportedFunctionTemplateParameters(Symbol* symbol) {
  if (symbol == NULL || !symbol->is_imported_module_symbol ||
      symbol->type == NULL || !TypeIsFunction(symbol->type) ||
      !symbol->flags.is_template) {
    return;
  }
  VectorDestructWithContents(
      &symbol->imported_function_template_parameters_backup,
      (VectorElementDestructor)TemplateParameterDelete,
      /*free_element=*/false);
  VectorInit(&symbol->imported_function_template_parameters_backup);
  Vector* live = &symbol->type->info.function.template_parameters;
  for (size_t i = 0; i < live->length; i++) {
    TemplateParameter* param = live->value.p[i];
    if (param != NULL) {
      VectorAppend(&symbol->imported_function_template_parameters_backup,
                   TemplateParameterCopy(param));
    }
  }
}

void SymbolRestoreImportedFunctionTemplateParameters(Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL || !TypeIsFunction(symbol->type) ||
      symbol->type->info.function.template_parameters.length > 0 ||
      symbol->imported_function_template_parameters_backup.length == 0) {
    return;
  }
  for (size_t i = 0;
       i < symbol->imported_function_template_parameters_backup.length; i++) {
    TemplateParameter* param =
        symbol->imported_function_template_parameters_backup.value.p[i];
    if (param != NULL) {
      VectorAppend(&symbol->type->info.function.template_parameters,
                   TemplateParameterCopy(param));
    }
  }
  if (symbol->type->info.function.template_parameters.length > 0) {
    symbol->type->info.function.template_parameter_count =
        (int)symbol->type->info.function.template_parameters.length;
  }
}

void SymbolDestruct(Symbol* symbol) {
  if (symbol == NULL || symbol->destruction_complete) {
    return;
  }
  // Set this before following owned links so cycles and shared imported graph
  // references make destruction idempotent.
  symbol->destruction_complete = true;
  StringDestruct(&symbol->name);
  StringDestruct(&symbol->asm_name);
  TypeRecordDelete(symbol->type);
  ASTNodeDelete(symbol->default_argument);
  ASTNodeDelete(symbol->constexpr_initializer);
  if (symbol->variable_template != NULL) {
    ASTNodeDelete(symbol->variable_template->initializer);
    VectorDestructWithContents(
        &symbol->variable_template->parameters,
        (VectorElementDestructor)TemplateParameterDelete,
        /*free_element=*/false);
    ConstraintExprDelete(symbol->variable_template->associated_constraint);
    VectorDestructWithContents(
        &symbol->variable_template->partial_specializations,
        (VectorElementDestructor)ClassTemplatePartialSpecializationDelete,
        /*free_element=*/false);
    free(symbol->variable_template);
    symbol->variable_template = NULL;
  }
  if (symbol->alias_template != NULL) {
    VectorDestructWithContents(
        &symbol->alias_template->parameters,
        (VectorElementDestructor)TemplateParameterDelete,
        /*free_element=*/false);
    free(symbol->alias_template);
    symbol->alias_template = NULL;
  }
  if (symbol->template_template_parameters != NULL) {
    VectorDeleteWithContents(
        symbol->template_template_parameters,
        (VectorElementDestructor)TemplateParameterDelete,
        /*free_element=*/false);
    symbol->template_template_parameters = NULL;
  }
  ConstraintExprDelete(symbol->associated_constraint);
  symbol->associated_constraint = NULL;
  StringDestruct(&symbol->owning_module_name);
  StringDestruct(&symbol->owning_module_partition);
  StringDestruct(&symbol->import_source_module);
  ConceptDelete(symbol->concept_definition);
  AttributeListDestruct(&symbol->attributes);
  VectorDestructWithContents(
      &symbol->imported_function_template_parameters_backup,
      (VectorElementDestructor)TemplateParameterDelete,
      /*free_element=*/false);
  free(symbol->cached_target_symbol_name);
  symbol->cached_target_symbol_name = NULL;
  if (symbol->overload_next != NULL) {
    SymbolDelete(symbol->overload_next);
  }
}

void SymbolDelete(Symbol* symbol) {
  if (symbol == NULL) {
    return;
  }
  bool graph_owned = symbol->is_imported_module_symbol;
  SymbolDestruct(symbol);
  if (!graph_owned) {
    free(symbol);
  }
}

void SymbolAddAttribute(Symbol* symbol, Attribute* attribute) {
  VectorAppend(&symbol->attributes, attribute);   // Takes ownership.
}

static bool CXXSymbolShouldMangle(Symbol* symbol) {
  if (!CompilerIsCXX() || symbol == NULL || symbol->type == NULL ||
      !TypeIsFunction(symbol->type)) {
    return false;
  }
  if (symbol->flags.is_c_linkage) {
    return false;
  }
  if (symbol->asm_name.length != 0) {
    return false;
  }
  if (strcmp(symbol->name.value, "main") == 0 &&
      symbol->type->info.function.cxx_member_owner == NULL &&
      symbol->namespace_ == NULL) {
    return false;
  }
  return true;
}

typedef struct {
  const char* name;
  const char* encoding;
} CXXOperatorEncodingEntry;

static int CompareCXXOperatorEncodingEntry(const void* key,
                                           const void* element) {
  const char* name = key;
  const CXXOperatorEncodingEntry* entry = element;
  return strcmp(name, entry->name);
}

static const char* CXXOperatorEncoding(const char* name) {
  static const CXXOperatorEncodingEntry entries[] = {
      {"operator co_await", "aw"},
      {"operator delete", "dl"},
      {"operator delete[]", "da"},
      {"operator new", "nw"},
      {"operator new[]", "na"},
      {"operator!", "nt"},
      {"operator!=", "ne"},
      {"operator%", "rm"},
      {"operator%=", "rM"},
      {"operator&", "an"},
      {"operator&&", "aa"},
      {"operator&=", "aN"},
      {"operator()", "cl"},
      {"operator*", "ml"},
      {"operator*=", "mL"},
      {"operator+", "pl"},
      {"operator++", "pp"},
      {"operator+=", "pL"},
      {"operator,", "cm"},
      {"operator-", "mi"},
      {"operator--", "mm"},
      {"operator-=", "mI"},
      {"operator->", "pt"},
      {"operator->*", "pm"},
      {"operator/", "dv"},
      {"operator/=", "dV"},
      {"operator<", "lt"},
      {"operator<<", "ls"},
      {"operator<<=", "lS"},
      {"operator<=", "le"},
      {"operator<=>", "ss"},
      {"operator=", "aS"},
      {"operator==", "eq"},
      {"operator>", "gt"},
      {"operator>=", "ge"},
      {"operator>>", "rs"},
      {"operator>>=", "rS"},
      {"operator[]", "ix"},
      {"operator^", "eo"},
      {"operator^=", "eO"},
      {"operator|", "or"},
      {"operator|=", "oR"},
      {"operator||", "oo"},
      {"operator~", "co"},
  };
  const CXXOperatorEncodingEntry* entry =
      bsearch(name, entries, sizeof(entries) / sizeof(entries[0]),
              sizeof(entries[0]), CompareCXXOperatorEncodingEntry);
  return entry != NULL ? entry->encoding : NULL;
}

static void AppendCXXNameComponent(String* out, const char* name) {
  const char* op_encoding = CXXOperatorEncoding(name);
  if (op_encoding != NULL) {
    StringAppend(out, op_encoding);
    return;
  }
  String sanitized;
  StringInit(&sanitized, NULL);
  bool needs_sanitizing = false;
  for (const char* p = name; *p != '\0'; p++) {
    char ch = *p;
    if (isalnum((unsigned char)ch) || ch == '_') {
      StringAppendChar(&sanitized, ch);
    } else {
      needs_sanitizing = true;
      StringAppendChar(&sanitized, '_');
    }
  }
  const char* component = needs_sanitizing ? sanitized.value : name;
  String length;
  StringInit(&length, NULL);
  StringPrintf(&length, "%zu", strlen(component));
  StringAppendString(out, &length);
  StringAppend(out, component);
  StringDestruct(&length);
  StringDestruct(&sanitized);
}

static void AppendCXXNestedNamespaceComponents(String* out, Namespace* ns) {
  if (ns == NULL || ns->parent == NULL) {
    return;
  }
  AppendCXXNestedNamespaceComponents(out, ns->parent);
  if (ns->name.length != 0) {
    AppendCXXNameComponent(out, ns->name.value);
  }
}

static void AppendCXXTypeEncoding(String* out, TypeRecord* type);
static void AppendCXXTaggedTypeName(String* out, Symbol* tag_symbol,
                                    String* tag_name, Struct* str);
static Namespace* CXXStructNamespace(Struct* str);
static void AppendCXXStructNameComponents(String* out, Struct* str);

static void AppendCXXTemplateObjectValue(String* out, ASTNode* initializer,
                                         TypeRecord* type) {
  if (initializer == NULL) {
    StringAppend(out, "Li0E");
    return;
  }
  if (initializer->op == AST_OP(expr_init)) {
    initializer = ((ExpressionInitializerASTNode*)initializer)->expr;
  }
  if (initializer != NULL && initializer->op == AST_OP(braced_init)) {
    BracedInitializerASTNode* braced =
        (BracedInitializerASTNode*)initializer;
    StringAppend(out, "tl");
    AppendCXXTypeEncoding(out, type);
    for (size_t i = 0; braced->initializers != NULL &&
                       i < braced->initializers->length; i++) {
      ASTNode* child = braced->initializers->value.p[i];
      TypeRecord* child_type = child != NULL && child->type != NULL
                                   ? child->type
                                   : type;
      if (child != NULL && child->op == AST_OP(expr_init) &&
          ((ExpressionInitializerASTNode*)child)->expr != NULL) {
        child_type = ((ExpressionInitializerASTNode*)child)->expr->type;
      }
      AppendCXXTemplateObjectValue(out, child, child_type);
    }
    StringAppendChar(out, 'E');
    return;
  }
  if (initializer != NULL && initializer->op == AST_OP(address)) {
    ASTNode* operand = ((UnaryASTNode*)initializer)->sub;
    if (operand != NULL && operand->op == AST_OP(identifier) &&
        ((IdentifierASTNode*)operand)->symbol != NULL) {
      Symbol* symbol = ((IdentifierASTNode*)operand)->symbol;
      StringAppend(out, "adL");
      if (symbol->asm_name.length != 0) {
        StringAppendString(out, &symbol->asm_name);
      } else {
        StringAppend(out, "_Z");
        AppendCXXNameComponent(out, symbol->name.value);
      }
      StringAppendChar(out, 'E');
      return;
    }
  }
  if (initializer != NULL && initializer->op == AST_OP(fnumber)) {
    ConstantASTNode* constant = (ConstantASTNode*)initializer;
    StringAppendChar(out, 'L');
    AppendCXXTypeEncoding(out, initializer->type != NULL
                                  ? initializer->type : type);
    if (TypeUsesFloat32Representation(initializer->type)) {
      float value = (float)constant->value.fvalue;
      uint32_t bits;
      memcpy(&bits, &value, sizeof(bits));
      StringPrintf(out, "%08x", (unsigned)bits);
    } else {
      double value = constant->value.fvalue;
      uint64_t bits;
      memcpy(&bits, &value, sizeof(bits));
      StringPrintf(out, "%016llx", (unsigned long long)bits);
    }
    StringAppendChar(out, 'E');
    return;
  }
  if (initializer != NULL && initializer->op == AST_OP(number)) {
    ConstantASTNode* constant = (ConstantASTNode*)initializer;
    long long value = constant->value.ivalue;
    StringAppendChar(out, 'L');
    AppendCXXTypeEncoding(out, initializer->type != NULL
                                  ? initializer->type : type);
    if (value < 0) {
      StringPrintf(out, "n%lld", -value);
    } else {
      StringPrintf(out, "%lld", value);
    }
    StringAppendChar(out, 'E');
    return;
  }
  // Keep unsupported structural subvalues collision-free while retaining a
  // valid vendor expression encoding.
  StringAppend(out, "u16dave_object_value");
}

Symbol* GetCXXTemplateParameterObject(TemplateArgument* argument) {
  if (argument == NULL || argument->type == NULL ||
      argument->object_initializer == NULL) {
    return NULL;
  }
  String name;
  StringInit(&name, "_ZTA");
  StringAppendChar(&name, 'X');
  AppendCXXTemplateObjectValue(&name, argument->object_initializer,
                               argument->type);
  StringAppendChar(&name, 'E');
  Symbol* symbol = NULL;
  for (size_t i = 0; compiler != NULL &&
                     i < compiler->initialized_static_variables.length; i++) {
    InitializedStaticVariable* existing =
        compiler->initialized_static_variables.value.p[i];
    if (existing != NULL && existing->symbol != NULL &&
        StringEqual(&existing->symbol->asm_name, name.value)) {
      symbol = existing->symbol;
      break;
    }
  }
  if (symbol == NULL) {
    TypeRecord* type = TypeRecordCopy(argument->type);
    type->qualifiers |= kQualConst;
    symbol = NewSymbol(name.value, type, STO(static));
    TypeRecordDelete(type);
    symbol->flags.invented = true;
    symbol->flags.is_defined = true;
    symbol->flags.is_weak = true;
    symbol->flags.is_constexpr = true;
    StringSetString(&symbol->asm_name, &name);
    CompilerRegisterTemplateParameterObject(
        symbol, ASTNodeClone(argument->object_initializer, IdentityCloneNode,
                             NULL, NULL));
  }
  StringDestruct(&name);
  return symbol;
}

static void AppendCXXTemplateNonTypeArgument(String* out,
                                             TemplateArgument* arg) {
  TemplateValueKind kind = TemplateArgumentConcreteValueKind(arg);
  if (kind == kTemplateValueIntegral || kind == kTemplateValueNone) {
    char value[64];
    long long int_value = arg->int_value;
    StringAppendChar(out, 'L');
    if (arg->type != NULL) {
      AppendCXXTypeEncoding(out, arg->type);
    } else {
      StringAppendChar(out, 'i');
    }
    if (int_value < 0) {
      snprintf(value, sizeof(value), "n%lldE", -int_value);
    } else {
      snprintf(value, sizeof(value), "%lldE", int_value);
    }
    StringAppend(out, value);
    return;
  }
  if (kind == kTemplateValueNull) {
    if (arg->type != NULL && TypeIsNullPointer(arg->type)) {
      StringAppend(out, "LDnE");
    } else {
      StringAppendChar(out, 'L');
      AppendCXXTypeEncoding(out, arg->type);
      StringAppend(out, "0E");
    }
    return;
  }
  if (kind == kTemplateValueObject) {
    StringAppendChar(out, 'X');
    AppendCXXTemplateObjectValue(out, arg->object_initializer, arg->type);
    StringAppendChar(out, 'E');
    return;
  }
  Symbol* symbol = kind == kTemplateValueMemberPointer &&
                           arg->member_function != NULL
                       ? arg->member_function
                       : arg->value_symbol;
  if (symbol == NULL) {
    StringAppend(out, "LDnE");
    return;
  }
  if (kind == kTemplateValueMemberPointer) {
    StringAppend(out, "XadL");
  } else {
    StringAppendChar(out, 'L');
  }
  if (kind == kTemplateValueMemberPointer &&
      symbol->asm_name.length == 0 && arg->type != NULL) {
    Struct* owner = TypeMemberPointerClass(arg->type);
    StringAppend(out, "_ZN");
    AppendCXXNestedNamespaceComponents(out, CXXStructNamespace(owner));
    AppendCXXStructNameComponents(out, owner);
    AppendCXXNameComponent(out, symbol->name.value);
    StringAppendChar(out, 'E');
  } else if (symbol->asm_name.length != 0) {
    StringAppendString(out, &symbol->asm_name);
  } else {
    StringAppend(out, "_Z");
    AppendCXXNameComponent(out, symbol->name.value);
  }
  StringAppendChar(out, 'E');
  if (kind == kTemplateValueMemberPointer) {
    StringAppendChar(out, 'E');
  }
}

static void AppendCXXTemplateTemplateArgument(String* out,
                                              TemplateArgument* arg) {
  if (arg == NULL) {
    return;
  }
  if (arg->template_parameter_index >= 0) {
    StringAppendChar(out, 'T');
    if (arg->template_parameter_index > 0) {
      StringPrintf(out, "%d", arg->template_parameter_index - 1);
    }
    StringAppendChar(out, '_');
    return;
  }
  Symbol* symbol = arg->template_symbol;
  if (symbol == NULL) {
    StringAppend(out, "Ut0_");
    return;
  }
  if (symbol->alias_template != NULL) {
    AppendCXXTaggedTypeName(out, symbol, &symbol->name, NULL);
    return;
  }
  Struct* str =
      symbol->type != NULL && TypeIsStructOrUnion(symbol->type)
          ? symbol->type->info.struct_info : NULL;
  AppendCXXTaggedTypeName(out, symbol, &symbol->name, str);
}

static void AppendCXXTemplateArgumentVector(String* out, Vector* args) {
  if (args == NULL) {
    return;
  }
  StringAppendChar(out, 'I');
  for (size_t i = 0; i < args->length; i++) {
    TemplateArgument* arg = args->value.p[i];
    if (arg == NULL) {
      continue;
    }
    if (arg->pack_arguments != NULL) {
      for (size_t j = 0; j < arg->pack_arguments->length; j++) {
        TemplateArgument* element = arg->pack_arguments->value.p[j];
        if (element == NULL) {
          continue;
        }
        if (element->kind == kTemplateParameterType) {
          AppendCXXTypeEncoding(out, element->type);
        } else if (element->kind == kTemplateParameterTemplate) {
          AppendCXXTemplateTemplateArgument(out, element);
        } else {
          AppendCXXTemplateNonTypeArgument(out, element);
        }
      }
      continue;
    }
    if (arg->kind == kTemplateParameterType) {
      AppendCXXTypeEncoding(out, arg->type);
    } else if (arg->kind == kTemplateParameterTemplate) {
      AppendCXXTemplateTemplateArgument(out, arg);
    } else {
      AppendCXXTemplateNonTypeArgument(out, arg);
    }
  }
  StringAppendChar(out, 'E');
}

static Namespace* CXXStructNamespace(Struct* str) {
  if (str == NULL) {
    return NULL;
  }
  if (str->lexical_parent != NULL) {
    return CXXStructNamespace(str->lexical_parent);
  }
  return str->tag_symbol != NULL ? str->tag_symbol->namespace_ : NULL;
}

static void AppendCXXStructNameComponents(String* out, Struct* str) {
  if (str == NULL) {
    return;
  }
  if (str->lexical_parent != NULL) {
    AppendCXXStructNameComponents(out, str->lexical_parent);
  }
  if (str->tag_name != NULL) {
    TypeRecord* tag_type =
        str->tag_symbol != NULL ? str->tag_symbol->type : NULL;
    if (tag_type != NULL && tag_type->template_origin != NULL) {
      AppendCXXNameComponent(out, tag_type->template_origin->name.value);
      AppendCXXTemplateArgumentVector(out, tag_type->template_arguments);
    } else {
      AppendCXXNameComponent(out, str->tag_name->value);
    }
  }
}

static void AppendCXXUnqualifiedName(String* out, Symbol* symbol) {
  TypeRecord* func = symbol->type;
  Struct* owner = func->info.function.cxx_member_owner;
  if (func->info.function.is_constructor) {
    if (owner == NULL || owner->tag_name == NULL) {
      AppendCXXNameComponent(out, symbol->name.value);
    }
    StringAppend(out, "C1");
    return;
  }
  if (func->info.function.is_destructor) {
    if (owner == NULL || owner->tag_name == NULL) {
      AppendCXXNameComponent(out, symbol->name.value);
    }
    StringAppend(out, "D1");
    return;
  }
  AppendCXXNameComponent(out, symbol->name.value);
}

static bool CXXNameNeedsNestedEncoding(Symbol* symbol) {
  return symbol->namespace_ != NULL ||
         symbol->type->info.function.cxx_member_owner != NULL;
}

static void AppendCXXName(String* out, Symbol* symbol) {
  Struct* owner = symbol->type->info.function.cxx_member_owner;
  if (!CXXNameNeedsNestedEncoding(symbol)) {
    AppendCXXUnqualifiedName(out, symbol);
    return;
  }

  // A member function of a class that lives in a namespace must carry that
  // namespace in its mangled name.  Compiler-synthesised special members set
  // symbol->namespace_ directly, but user-declared members only record their
  // owning class, so fall back to the class tag symbol's namespace (as
  // SymbolSetCXXDataAsmName does for static data members).
  Namespace* ns = symbol->namespace_;
  if (ns == NULL && owner != NULL) {
    ns = CXXStructNamespace(owner);
  }

  StringAppendChar(out, 'N');
  if (symbol->type->info.function.has_explicit_object_parameter) {
    StringAppendChar(out, 'H');
  } else {
    if (symbol->type->info.function.is_volatile_member) {
      StringAppendChar(out, 'V');
    }
    if (symbol->type->info.function.is_const_member) {
      StringAppendChar(out, 'K');
    }
    if (symbol->type->info.function.ref_qualifier == kCXXRefQualifierLValue) {
      StringAppendChar(out, 'R');
    } else if (symbol->type->info.function.ref_qualifier ==
               kCXXRefQualifierRValue) {
      StringAppendChar(out, 'O');
    }
  }
  AppendCXXNestedNamespaceComponents(out, ns);
  if (owner != NULL) {
    AppendCXXStructNameComponents(out, owner);
  }
  AppendCXXUnqualifiedName(out, symbol);
  StringAppendChar(out, 'E');
}

// Mangles a class/union/enum type name.  A type that lives in a namespace must
// use the Itanium nested-name form (`N <namespace-components> <tag> E`) so that,
// e.g., a parameter of type `std::nothrow_t` encodes as `N3std9nothrow_tE`
// rather than the unqualified `9nothrow_t`.  Types at global scope keep the
// bare `<length><name>` component.
static void AppendCXXTaggedTypeName(String* out, Symbol* tag_symbol,
                                    String* tag_name, Struct* str) {
  Namespace* ns = str != NULL ? CXXStructNamespace(str)
                              : tag_symbol != NULL ? tag_symbol->namespace_ : NULL;
  if (ns != NULL) {
    StringAppendChar(out, 'N');
    AppendCXXNestedNamespaceComponents(out, ns);
    if (str != NULL) {
      AppendCXXStructNameComponents(out, str);
    } else {
      AppendCXXNameComponent(out, tag_name->value);
    }
    StringAppendChar(out, 'E');
  } else if (str != NULL && str->lexical_parent != NULL) {
    StringAppendChar(out, 'N');
    AppendCXXStructNameComponents(out, str);
    StringAppendChar(out, 'E');
  } else {
    AppendCXXNameComponent(out, tag_name->value);
  }
}

static void AppendCXXTypeEncoding(String* out, TypeRecord* type) {
  if (type == NULL) {
    StringAppendChar(out, 'v');
    return;
  }
  if (TypeIsConst(type)) {
    StringAppendChar(out, 'K');
  }
  if (TypeIsVolatile(type)) {
    StringAppendChar(out, 'V');
  }
  switch (type->declarator) {
    case kDeclPointer:
      StringAppendChar(out, 'P');
      AppendCXXTypeEncoding(out, type->next);
      return;
    case kDeclReference:
      StringAppendChar(out, 'R');
      AppendCXXTypeEncoding(out, type->next);
      return;
    case kDeclRValueReference:
      StringAppendChar(out, 'O');
      AppendCXXTypeEncoding(out, type->next);
      return;
    case kDeclMemberPointer: {
      StringAppendChar(out, 'M');
      Struct* owner = type->info.struct_info;
      if (owner != NULL && owner->tag_name != NULL) {
        AppendCXXTaggedTypeName(out, owner->tag_symbol, owner->tag_name, owner);
      } else {
        StringAppendChar(out, 'v');
      }
      AppendCXXTypeEncoding(out, type->next);
      return;
    }
    case kDeclArray:
      // Itanium ABI array encoding is `A <bound> _ <element>`.  Encoding an
      // array as a pointer aliases `T(&)[N]` with `T*&`, which can make template
      // instantiation caching emit one specialization while a call references
      // the other.
      StringAppendChar(out, 'A');
      if (!type->info.array.is_flexible && !type->info.array.is_vla &&
          !type->info.array.is_placeholder_vla &&
          !type->info.array.is_dependent_bound) {
        char bound[32];
        snprintf(bound, sizeof(bound), "%d", type->info.array.size.fixed);
        StringAppend(out, bound);
      }
      StringAppendChar(out, '_');
      AppendCXXTypeEncoding(out, type->next);
      return;
    case kDeclFunction:
      StringAppendChar(out, 'F');
      AppendCXXTypeEncoding(out, type->next);
      for (size_t i = 0; i < type->info.function.prototype.length; i++) {
        Symbol* formal = type->info.function.prototype.value.p[i];
        AppendCXXTypeEncoding(out, formal->type);
      }
      StringAppendChar(out, 'E');
      return;
    case kDeclPrimitive:
      break;
  }

  if (TypeIsVoid(type)) {
    StringAppendChar(out, 'v');
  } else if (TypeIsBool(type)) {
    StringAppendChar(out, 'b');
  } else if (TypeIsReflection(type)) {
    StringAppend(out, "u17__reflection_type");
  } else if (TypeIsChar8(type)) {
    StringAppend(out, "Du");
  } else if (TypeIsChar16(type)) {
    StringAppend(out, "Ds");
  } else if (TypeIsChar32(type)) {
    StringAppend(out, "Di");
  } else if (TypeIsChar(type)) {
    StringAppendChar(out, TypeIsUnsigned(type) ? 'h' : 'c');
  } else if (TypeIsShort(type)) {
    StringAppendChar(out, TypeIsUnsigned(type) ? 't' : 's');
  } else if (TypeIsLongLong(type)) {
    StringAppend(out, TypeIsUnsigned(type) ? "y" : "x");
  } else if (TypeIsLong(type)) {
    StringAppendChar(out, TypeIsUnsigned(type) ? 'm' : 'l');
  } else if (TypeIsInt(type)) {
    StringAppendChar(out, TypeIsUnsigned(type) ? 'j' : 'i');
  } else if (TypeIsFloat32(type)) {
    StringAppend(out, "DF32_");
  } else if (TypeIsFloat64(type)) {
    StringAppend(out, "DF64_");
  } else if (TypeIsFloat(type)) {
    StringAppendChar(out, 'f');
  } else if (TypeIsDouble(type)) {
    StringAppendChar(out, 'd');
  } else if (TypeIsLongDouble(type)) {
    StringAppendChar(out, 'e');
  } else if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL &&
             type->info.struct_info->tag_name != NULL) {
    AppendCXXTaggedTypeName(out, type->info.struct_info->tag_symbol,
                            type->info.struct_info->tag_name,
                            type->info.struct_info);
  } else if (TypeIsEnum(type) && type->info.enum_info != NULL &&
             type->info.enum_info->tag_name != NULL) {
    AppendCXXTaggedTypeName(out, type->info.enum_info->tag_symbol,
                            type->info.enum_info->tag_name, NULL);
  } else {
    StringAppendChar(out, 'v');
  }
}

void AppendCXXMangledTypeName(String* out, TypeRecord* type) {
  AppendCXXTypeEncoding(out, type);
}

static void AppendCXXFunctionParameterTypes(String* out, Symbol* symbol) {
  TypeRecord* func = symbol->type;
  bool has_implicit_this =
      func->info.function.prototype.length > 0 &&
      strcmp(((Symbol*)func->info.function.prototype.value.p[0])->name.value,
             "this") == 0;
  size_t first_arg = has_implicit_this ? 1 : 0;
  if (func->info.function.prototype.length <= first_arg) {
    StringAppendChar(out, 'v');
    return;
  }
  for (size_t i = first_arg; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    AppendCXXTypeEncoding(out, formal->type);
  }
}

static void AppendCXXTemplateArguments(String* out, Symbol* symbol) {
  TypeRecord* func = symbol->type;
  if (!TypeIsFunction(func) ||
      func->info.function.template_origin == NULL ||
      func->template_arguments == NULL) {
    return;
  }
  StringAppendChar(out, 'I');
  for (size_t i = 0; i < func->template_arguments->length; i++) {
    TemplateArgument* arg = func->template_arguments->value.p[i];
    if (arg->pack_arguments != NULL) {
      for (size_t j = 0; j < arg->pack_arguments->length; j++) {
        TemplateArgument* element = arg->pack_arguments->value.p[j];
        if (element->kind == kTemplateParameterType) {
          AppendCXXTypeEncoding(out, element->type);
        } else if (element->kind == kTemplateParameterTemplate) {
          AppendCXXTemplateTemplateArgument(out, element);
        } else {
          AppendCXXTemplateNonTypeArgument(out, element);
        }
      }
      continue;
    }
    if (arg->kind == kTemplateParameterType) {
      AppendCXXTypeEncoding(out, arg->type);
    } else if (arg->kind == kTemplateParameterTemplate) {
      AppendCXXTemplateTemplateArgument(out, arg);
    } else {
      AppendCXXTemplateNonTypeArgument(out, arg);
    }
  }
  StringAppendChar(out, 'E');
}

void SymbolSetCXXMangledAsmName(Symbol* symbol) {
  if (!CXXSymbolShouldMangle(symbol)) {
    return;
  }
  String mangled;
  StringInit(&mangled, NULL);
  if (compiler->prepend_underscore) {
    StringAppendChar(&mangled, '_');
  }
  StringAppend(&mangled, "_Z");
  AppendCXXModuleIdentityMangling(&mangled, symbol);
  AppendCXXName(&mangled, symbol);
  AppendCXXTemplateArguments(&mangled, symbol);
  AppendCXXFunctionParameterTypes(&mangled, symbol);
  StringSetString(&symbol->asm_name, &mangled);
  StringDestruct(&mangled);
  // The cached target name derives from asm_name; drop it so it is recomputed.
  free(symbol->cached_target_symbol_name);
  symbol->cached_target_symbol_name = NULL;
}

void SymbolSetCXXDataAsmName(Symbol* symbol, Struct* owner) {
  if (!CompilerIsCXX() || symbol == NULL || symbol->asm_name.length != 0 ||
      symbol->flags.is_c_linkage) {
    return;
  }

  String mangled;
  StringInit(&mangled, NULL);
  if (compiler->prepend_underscore) {
    StringAppendChar(&mangled, '_');
  }
  StringAppend(&mangled, "_Z");
  AppendCXXModuleIdentityMangling(&mangled, symbol);

  Namespace* ns = symbol->namespace_;
  if (ns == NULL && owner != NULL && owner->tag_symbol != NULL) {
    ns = owner->tag_symbol->namespace_;
  }

  bool nested = ns != NULL ||
                (owner != NULL && owner->tag_name != NULL);
  if (nested) {
    StringAppendChar(&mangled, 'N');
    AppendCXXNestedNamespaceComponents(&mangled, ns);
    if (owner != NULL && owner->tag_name != NULL) {
      AppendCXXNameComponent(&mangled, owner->tag_name->value);
    }
    AppendCXXNameComponent(&mangled, symbol->name.value);
    StringAppendChar(&mangled, 'E');
  } else {
    AppendCXXNameComponent(&mangled, symbol->name.value);
  }

  StringSetString(&symbol->asm_name, &mangled);
  StringDestruct(&mangled);
  // The cached target name derives from asm_name; drop it so it is recomputed.
  free(symbol->cached_target_symbol_name);
  symbol->cached_target_symbol_name = NULL;
}

Symbol* SymbolClone(Symbol* sym) {
  Symbol* new_sym = NewSymbol(sym->name.value, sym->type, sym->storage);
  new_sym->flags = sym->flags;
  new_sym->concept_definition = NULL;
  new_sym->associated_constraint =
      ConceptsCloneConstraint(sym->associated_constraint);
  new_sym->cxx_linkage = sym->cxx_linkage;
  StringSetString(&new_sym->owning_module_name, &sym->owning_module_name);
  StringSetString(&new_sym->owning_module_partition, &sym->owning_module_partition);
  StringSetString(&new_sym->import_source_module, &sym->import_source_module);
  new_sym->usage_info = sym->usage_info;
  new_sym->value = sym->value;
  new_sym->stack_offset = sym->stack_offset;
  new_sym->alias_target = sym->alias_target;
  new_sym->lambda_capture_source = sym->lambda_capture_source;
  new_sym->lambda_capture_by_reference =
      sym->lambda_capture_by_reference;
  new_sym->overload_next = NULL;
  new_sym->default_argument =
      ASTNodeClone(sym->default_argument, IdentityCloneNode, NULL, NULL);
  new_sym->constexpr_initializer =
      ASTNodeClone(sym->constexpr_initializer, IdentityCloneNode, NULL, NULL);
  new_sym->is_constexpr_representable = sym->is_constexpr_representable;
  new_sym->requires_ast_constexpr = sym->requires_ast_constexpr;
  new_sym->constexpr_reference_scope = sym->constexpr_reference_scope;
  new_sym->template_template_parameters =
      TemplateParameterVectorCopy(sym->template_template_parameters);
  new_sym->template_template_parameter_kind =
      sym->template_template_parameter_kind;
  new_sym->location = sym->location;
  new_sym->alignment = sym->alignment;
  new_sym->template_parameter_index = sym->template_parameter_index;
  new_sym->dependent_value_template_parameter_index =
      sym->dependent_value_template_parameter_index;
  new_sym->structured_binding_pack_size =
      sym->structured_binding_pack_size;
  new_sym->namespace_ = sym->namespace_;
  StringSetString(&new_sym->asm_name, &sym->asm_name);
  // NewSymbol already initialized new_sym->attributes; replace it with a deep
  // copy of the source's attributes.
  VectorDestruct(&new_sym->attributes);
  AttributeListClone(&new_sym->attributes, &sym->attributes);
  return new_sym;
}

bool SymbolHasAttribute(Symbol* symbol, const char* attribute) {
  return AttributeListHas(&symbol->attributes, attribute);
}

Attribute* SymbolFindAttribute(Symbol* symbol, const char* attribute) {
  return AttributeListFind(&symbol->attributes, attribute);
}

bool SymbolHasWeakBinding(Symbol* symbol) {
  return symbol != NULL && symbol->flags.is_weak;
}

static const char* storages[] = {
  "",
  "auto ",
  "static ",
  "typedef ",
  "extern ",
  "register ",
  "assembler ",
  "__thread ",
};

void SymbolPrintDetails(Symbol* sym, bool with_function_body, FILE* fp) {
  String storage;
  StringInit(&storage, "");
  for (int i = 0; i < 32; i++) {
    if (StorageIs(sym->storage, 1<<i)) {
      if (1<<i == STO(thread)) {
        StringAppend(&storage, CompilerIsCXX() ? "thread_local " : "__thread ");
      } else {
        StringAppend(&storage, storages[i]);
      }
    }
  }
 
  fprintf(fp, "%s: %s", sym->name.value, storage.value);
  TypeRecordPrintDetails(sym->type, with_function_body, fp);
  StringDestruct(&storage);
}

void SymbolPrint(Symbol* sym, FILE* fp) { SymbolPrintDetails(sym, false, fp); }

void SymbolSetType(Symbol* symbol, struct TypeRecord* type) {
  if (symbol->type == type) {
    // Already set
    return;
  }
  if (symbol->type != NULL) {
    TypeRecordDelete(symbol->type);
  }
  symbol->type = type;
  TypeRecordIncRef(type);
}
