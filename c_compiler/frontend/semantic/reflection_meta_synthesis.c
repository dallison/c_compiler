//
//  reflection_meta_synthesis.c
//  c_compiler
//

#include "reflection_meta_synthesis.h"

#include "compiler.h"
#include "constexpr.h"
#include "errors.h"
#include "expr_evaluator.h"
#include "expr_semantics.h"
#include "init_semantics.h"
#include "reflection.h"
#include "reflection_semantics.h"
#include "symbol.h"
#include "symbol_table.h"
#include "syntax.h"
#include "type_class_internal.h"
#include "type_compare.h"
#include "type_internal.h"
#include "type_template.h"
#include "vector.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  kMetaSynthUnknown,
  kMetaSynthReflectConstant,
  kMetaSynthReflectObject,
  kMetaSynthReflectFunction,
  kMetaSynthExtract,
  kMetaSynthCanSubstitute,
  kMetaSynthSubstitute,
  kMetaSynthDataMemberSpec,
  kMetaSynthIsDataMemberSpec,
  kMetaSynthDefineAggregate,
  kMetaSynthReflectConstantString,
  kMetaSynthReflectConstantArray,
  kMetaSynthDefineStaticString,
  kMetaSynthDefineStaticArray,
  kMetaSynthDefineStaticObject,
  kMetaSynthIsStringLiteral,
} MetaSynthesisOperation;

typedef struct {
  const char* name;
  MetaSynthesisOperation operation;
} MetaSynthesisOperationEntry;

static const MetaSynthesisOperationEntry kMetaSynthesisOperations[] = {
    {"can_substitute", kMetaSynthCanSubstitute},
    {"data_member_spec", kMetaSynthDataMemberSpec},
    {"define_aggregate", kMetaSynthDefineAggregate},
    {"define_static_array", kMetaSynthDefineStaticArray},
    {"define_static_object", kMetaSynthDefineStaticObject},
    {"define_static_string", kMetaSynthDefineStaticString},
    {"extract", kMetaSynthExtract},
    {"is_data_member_spec", kMetaSynthIsDataMemberSpec},
    {"is_string_literal", kMetaSynthIsStringLiteral},
    {"reflect_constant", kMetaSynthReflectConstant},
    {"reflect_constant_array", kMetaSynthReflectConstantArray},
    {"reflect_constant_string", kMetaSynthReflectConstantString},
    {"reflect_function", kMetaSynthReflectFunction},
    {"reflect_object", kMetaSynthReflectObject},
    {"substitute", kMetaSynthSubstitute},
};

static MetaSynthesisOperation MetaSynthesisOperationForName(const char* name) {
  size_t begin = 0;
  size_t end = sizeof(kMetaSynthesisOperations) / sizeof(kMetaSynthesisOperations[0]);
  while (begin < end) {
    size_t middle = begin + (end - begin) / 2;
    int comparison = strcmp(name, kMetaSynthesisOperations[middle].name);
    if (comparison == 0) {
      return kMetaSynthesisOperations[middle].operation;
    }
    if (comparison < 0) {
      end = middle;
    } else {
      begin = middle + 1;
    }
  }
  return kMetaSynthUnknown;
}

static Symbol* MetaSynthesisCallSymbol(VectorASTNode* call) {
  if (call == NULL || call->left == NULL ||
      call->left->op != AST_OP(identifier)) {
    return NULL;
  }
  Symbol* symbol = ((IdentifierASTNode*)call->left)->symbol;
  for (size_t depth = 0;
       symbol != NULL && symbol->alias_target != NULL && depth < 64;
       depth++) {
    symbol = symbol->alias_target;
  }
  for (Symbol* candidate = symbol; candidate != NULL;
       candidate = candidate->overload_next) {
    if (SymbolHasAttribute(candidate, "meta_intrinsic")) {
      return candidate;
    }
  }
  return NULL;
}

static ASTNode* MetaSynthesisBool(bool value, SourceLocation location) {
  return NewIntConstantASTNode(value ? 1 : 0,
                               NewTypeRecordWithSize(kTypeBool, kQualConst),
                               location);
}

static ReflectionValue* MetaSynthesisEvaluateReflection(ASTNode* expression) {
  if (expression == NULL) {
    return NULL;
  }
  ReflectionValue* value =
      SemanticReflectionValueFromExpression(expression);
  if (value != NULL) {
    return value;
  }
  ConstEvalContext context;
  ConstEvalContextInit(&context);
  value = ConstexprEvaluateReflectionExpression(&context, expression);
  ConstEvalContextDestruct(&context);
  return value;
}

static Symbol* MetaSynthesisReferencedSymbol(ASTNode* expression) {
  for (size_t depth = 0; expression != NULL && depth < 64; depth++) {
    if (expression->op == AST_OP(identifier)) {
      return ((IdentifierASTNode*)expression)->symbol;
    }
    if (expression->op == AST_OP(cast)) {
      expression = ((CastASTNode*)expression)->expr;
      continue;
    }
    if (expression->op == AST_OP(expr_init)) {
      expression = ((ExpressionInitializerASTNode*)expression)->expr;
      continue;
    }
    if (expression->op == AST_OP(comma)) {
      expression = ((BinaryASTNode*)expression)->right;
      continue;
    }
    break;
  }
  return NULL;
}

static bool MetaSynthesisReflectionRange(VectorASTNode* call, size_t arg_index,
                                         Vector* out_values) {
  if (call == NULL || call->children == NULL ||
      arg_index >= call->children->length) {
    return false;
  }
  ASTNode* arg = call->children->value.p[arg_index];
  if (arg == NULL) {
    return false;
  }
  if (arg->op == AST_OP(braced_init)) {
    BracedInitializerASTNode* braced = (BracedInitializerASTNode*)arg;
    if (braced->initializers->length >= 2) {
      ASTNode* array = braced->initializers->value.p[1];
      if (array != NULL && array->op == AST_OP(braced_init)) {
        BracedInitializerASTNode* elements = (BracedInitializerASTNode*)array;
        for (size_t i = 0; i < elements->initializers->length; i++) {
          ReflectionValue* value = MetaSynthesisEvaluateReflection(
              elements->initializers->value.p[i]);
          if (value == NULL) {
            return false;
          }
          VectorAppend(out_values, value);
        }
        return true;
      }
    }
    for (size_t i = 0; i < braced->initializers->length; i++) {
      ReflectionValue* value =
          MetaSynthesisEvaluateReflection(braced->initializers->value.p[i]);
      if (value == NULL) {
        return false;
      }
      VectorAppend(out_values, value);
    }
    return true;
  }
  ReflectionValue* single = MetaSynthesisEvaluateReflection(arg);
  if (single == NULL) {
    return false;
  }
  VectorAppend(out_values, single);
  return true;
}

static TypeRecord* MetaSynthesisFunctionTemplateContext(void) {
  if (compiler->current_function != NULL) {
    return compiler->current_function;
  }
  if (compiler->functions_being_analyzed.length > 0) {
    return compiler->functions_being_analyzed.value.p[
        compiler->functions_being_analyzed.length - 1];
  }
  return NULL;
}

static TypeRecord* MetaSynthesisResolveTemplateArgumentType(
    TemplateArgument* arg) {
  if (arg == NULL) {
    return NULL;
  }
  if (arg->kind == kTemplateParameterType && arg->type != NULL) {
    return arg->type;
  }
  if (arg->template_parameter_index >= 0) {
    TypeRecord* context = MetaSynthesisFunctionTemplateContext();
    if (context != NULL && context->template_arguments != NULL) {
      size_t index = (size_t)arg->template_parameter_index;
      if (index < context->template_arguments->length) {
        TemplateArgument* bound = context->template_arguments->value.p[index];
        if (bound != NULL && bound->kind == kTemplateParameterType &&
            bound->type != NULL) {
          return bound->type;
        }
      }
    }
  }
  if (arg->dependent_expr != NULL) {
    ASTNode* expr = AnalyzeExpression(arg->dependent_expr);
    if (expr != NULL && expr->type != NULL) {
      return expr->type;
    }
  }
  return NULL;
}

static TemplateArgument* MetaSynthesisTemplateArgumentFromReflection(
    ReflectionValue* reflection) {
  TemplateArgument* arg = calloc(1, sizeof(*arg));
  arg->location = reflection != NULL ? reflection->location
                                     : SOURCE_LOCATION_MISSING;
  if (reflection == NULL) {
    return arg;
  }
  if (reflection->sequence.length > 0) {
    arg->pack_arguments = NewVector();
    for (size_t i = 0; i < reflection->sequence.length; i++) {
      ReflectionValue* element = reflection->sequence.value.p[i];
      TemplateArgument* child =
          MetaSynthesisTemplateArgumentFromReflection(element);
      if (child != NULL) {
        VectorAppend(arg->pack_arguments, child);
      }
    }
    arg->is_pack_expansion = arg->pack_arguments->length > 0;
    return arg;
  }
  switch (reflection->kind) {
    case kReflectionType:
    case kReflectionTypeAlias:
      arg->kind = kTemplateParameterType;
      arg->type = ReflectionValueType(reflection);
      break;
    case kReflectionValue:
      if (reflection->reflected_type != NULL &&
          TypeIsIntegral(reflection->reflected_type)) {
        arg->kind = kTemplateParameterNonType;
        arg->value_kind = kTemplateValueIntegral;
        arg->int_value = reflection->scalar_ivalue;
        arg->type = TypeRecordCopy(reflection->reflected_type);
      } else {
        arg->kind = kTemplateParameterNonType;
        arg->value_kind = kTemplateValueReflection;
        arg->reflection_value = reflection;
        arg->type = TypeRecordCopy(reflection->reflected_type);
      }
      break;
    default:
      arg->kind = kTemplateParameterNonType;
      arg->value_kind = kTemplateValueReflection;
      arg->reflection_value = reflection;
      if (reflection->reflected_type != NULL) {
        arg->type = TypeRecordCopy(reflection->reflected_type);
      } else if (reflection->symbol != NULL &&
                 reflection->symbol->type != NULL) {
        arg->type = TypeRecordCopy(reflection->symbol->type);
      }
      break;
  }
  return arg;
}

static Vector* MetaSynthesisTemplateArgumentsFromRange(Vector* reflections) {
  Vector* args = NewVector();
  for (size_t i = 0; i < reflections->length; i++) {
    ReflectionValue* reflection = reflections->value.p[i];
    VectorAppend(args, MetaSynthesisTemplateArgumentFromReflection(reflection));
  }
  return args;
}

static bool MetaSynthesisParseOptionalBool(ASTNode* node, bool* out_value) {
  if (node == NULL) {
    return false;
  }
  int64_t value = 0;
  if (EvaluateIntegerExpression(node, &value)) {
    *out_value = value != 0;
    return true;
  }
  return false;
}

static bool MetaSynthesisParseDataMemberOptions(ASTNode* options,
                                                TypeRecord* default_member_type,
                                                ReflectionDataMemberSpec** out_spec,
                                                SourceLocation location) {
  if (options == NULL || out_spec == NULL) {
    return false;
  }
  TypeRecord* member_type =
      default_member_type != NULL ? TypeRecordCopy(default_member_type) : NULL;
  if (options->op == AST_OP(braced_init)) {
    BracedInitializerASTNode* braced = (BracedInitializerASTNode*)options;
    for (size_t i = 0; i < braced->initializers->length; i++) {
      ASTNode* entry = braced->initializers->value.p[i];
      if (entry == NULL || entry->op != AST_OP(designated_init)) {
        continue;
      }
      DesignatedInitializerASTNode* designated =
          (DesignatedInitializerASTNode*)entry;
      if (designated->designators == NULL ||
          designated->designators->length == 0) {
        continue;
      }
      Designator* designator = designated->designators->value.p[0];
      const char* field = NULL;
      if (designator != NULL && designator->designator_type == kDesignatorStruct) {
        if (designator->is_resolved_member &&
            designator->value.struct_member != NULL &&
            designator->value.struct_member->symbol != NULL) {
          field = designator->value.struct_member->symbol->name.value;
        } else if (!designator->is_resolved_member &&
                   designator->value.struct_member_name != NULL) {
          field = designator->value.struct_member_name->value;
        }
      }
      if (field == NULL) {
        continue;
      }
      if (strcmp(field, "type") == 0 || strcmp(field, "member_type") == 0) {
        ReflectionValue* type_value =
            MetaSynthesisEvaluateReflection(designated->init);
        if (type_value != NULL) {
          member_type = SemanticMaterializeReflectedType(type_value, location);
        }
      }
    }
  }
  if (member_type == NULL) {
    return false;
  }
  ReflectionDataMemberSpec* spec = ReflectionDataMemberSpecNew(member_type);
  TypeRecordDelete(member_type);
  if (options->op == AST_OP(braced_init)) {
    BracedInitializerASTNode* braced = (BracedInitializerASTNode*)options;
    for (size_t i = 0; i < braced->initializers->length; i++) {
      ASTNode* entry = braced->initializers->value.p[i];
      if (entry == NULL || entry->op != AST_OP(designated_init)) {
        continue;
      }
      DesignatedInitializerASTNode* designated =
          (DesignatedInitializerASTNode*)entry;
      if (designated->designators == NULL ||
          designated->designators->length == 0) {
        continue;
      }
      Designator* designator = designated->designators->value.p[0];
      const char* field = NULL;
      if (designator != NULL && designator->designator_type == kDesignatorStruct) {
        if (designator->is_resolved_member &&
            designator->value.struct_member != NULL &&
            designator->value.struct_member->symbol != NULL) {
          field = designator->value.struct_member->symbol->name.value;
        } else if (!designator->is_resolved_member &&
                   designator->value.struct_member_name != NULL) {
          field = designator->value.struct_member_name->value;
        }
      }
      if (field == NULL) {
        continue;
      }
      if (strcmp(field, "name") == 0) {
        if (designated->init != NULL &&
            designated->init->op == AST_OP(string)) {
          String* text = ((ConstantASTNode*)designated->init)->value.string;
          if (text != NULL && text->value != NULL) {
            StringSetString(&spec->name, text);
            spec->has_name = true;
          }
        }
      } else if (strcmp(field, "alignment") == 0) {
        int64_t alignment = 0;
        if (EvaluateIntegerExpression(designated->init, &alignment) &&
            alignment > 0) {
          spec->alignment = (size_t)alignment;
          spec->has_alignment = true;
        }
      } else if (strcmp(field, "bit_width") == 0) {
        int64_t bit_width = 0;
        if (EvaluateIntegerExpression(designated->init, &bit_width) &&
            bit_width > 0) {
          spec->bit_width = (size_t)bit_width;
          spec->has_bit_width = true;
        }
      } else if (strcmp(field, "no_unique_address") == 0) {
        MetaSynthesisParseOptionalBool(designated->init,
                                       &spec->no_unique_address);
      } else if (strcmp(field, "annotations") == 0) {
        Vector values;
        VectorInit(&values);
        if (designated->init != NULL &&
            designated->init->op == AST_OP(braced_init)) {
          BracedInitializerASTNode* items =
              (BracedInitializerASTNode*)designated->init;
          for (size_t j = 0; j < items->initializers->length; j++) {
            ReflectionValue* annotation = MetaSynthesisEvaluateReflection(
                items->initializers->value.p[j]);
            if (annotation != NULL) {
              VectorAppend(&spec->annotations, annotation);
            }
          }
        }
        VectorDestruct(&values);
      }
    }
  }
  *out_spec = spec;
  return true;
}

static StructMember* MetaSynthesisFindMatchingMember(Struct* str,
                                                     ReflectionDataMemberSpec* spec,
                                                     size_t index) {
  if (str == NULL || spec == NULL) {
    return NULL;
  }
  if (spec->has_name && spec->name.value != NULL) {
    StructMember* member = FindStructMemberByName(str, spec->name.value);
    return member;
  }
  size_t data_index = 0;
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->is_static || member->is_member_function ||
        member->is_using_declaration) {
      continue;
    }
    if (data_index == index) {
      return member;
    }
    data_index++;
  }
  return NULL;
}

static bool MetaSynthesisMemberMatchesSpec(StructMember* member,
                                           ReflectionDataMemberSpec* spec) {
  if (member == NULL || member->symbol == NULL || spec == NULL) {
    return false;
  }
  if (!TypeEqual(member->symbol->type, spec->member_type)) {
    return false;
  }
  if (spec->has_name) {
    if (member->symbol->name.value == NULL ||
        !StringEqual(&member->symbol->name, spec->name.value)) {
      return false;
    }
  }
  if (spec->has_alignment &&
      (size_t)member->symbol->alignment != spec->alignment) {
    return false;
  }
  if (spec->has_bit_width && (size_t)member->bit_size != spec->bit_width) {
    return false;
  }
  if (spec->no_unique_address &&
      !AttributeListHas(&member->symbol->attributes, "no_unique_address")) {
    return false;
  }
  if (spec->annotations.length > 0) {
    if (member->symbol->attributes.length < spec->annotations.length) {
      return false;
    }
  }
  return true;
}

static void MetaSynthesisRollbackMembers(Struct* str, size_t old_count) {
  while (str->members.length > old_count) {
    StructMember* member = str->members.value.p[str->members.length - 1];
    VectorPop(&str->members);
    StructMemberDelete(member);
  }
  StructRebuildMemberLookupTables(str);
}

static bool MetaSynthesisApplyDataMemberSpec(Struct* str,
                                             ReflectionDataMemberSpec* spec,
                                             size_t index,
                                             SourceLocation location) {
  StructMember* existing = MetaSynthesisFindMatchingMember(str, spec, index);
  if (existing != NULL) {
    return MetaSynthesisMemberMatchesSpec(existing, spec);
  }
  String member_name;
  StringInit(&member_name, NULL);
  if (spec->has_name && spec->name.value != NULL) {
    StringSetString(&member_name, &spec->name);
  } else {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "__meta_member_%zu", index);
    StringSet(&member_name, buffer);
  }
  Symbol* member_symbol =
      NewSymbol(member_name.value, TypeRecordCopy(spec->member_type), STO(implicit));
  member_symbol->namespace_ = str->tag_symbol != NULL
                                  ? str->tag_symbol->namespace_
                                  : NULL;
  member_symbol->flags.is_defined = true;
  member_symbol->location = location;
  if (spec->has_alignment) {
    member_symbol->alignment = (int)spec->alignment;
  }
  if (spec->no_unique_address) {
    VectorAppend(&member_symbol->attributes, NewAttribute("no_unique_address"));
  }
  for (size_t i = 0; i < spec->annotations.length; i++) {
    ReflectionValue* annotation = spec->annotations.value.p[i];
    if (annotation != NULL && annotation->annotation != NULL &&
        annotation->annotation->name.value != NULL) {
      VectorAppend(&member_symbol->attributes,
                   NewAttribute(annotation->annotation->name.value));
    }
  }
  StructMember* member = NewStructMember(member_symbol);
  member->access = kAccessPublic;
  if (spec->has_bit_width) {
    TypeParser parser;
    TypeParserInit(&parser, compiler->syntax.lex, &compiler->syntax,
                   STO(implicit), compiler->syntax.context);
    ParseBitField(&parser, str->is_union, str, member_symbol, member);
    member->bit_size = (int)spec->bit_width;
    TypeParserDestruct(&parser);
  } else {
    StructAddSyntheticMember(str, member);
  }
  StringDestruct(&member_name);
  return true;
}

static void MetaSynthesisCompleteClassAfterAggregate(Struct* str) {
  if (str == NULL || str->tag_symbol == NULL) {
    return;
  }
  Symbol* tag = str->tag_symbol;
  TypeParser parser;
  TypeParserInit(&parser, compiler->syntax.lex, &compiler->syntax,
                 STO(implicit), compiler->syntax.context);
  AddImplicitCXXSpecialMembers(&parser, str, tag);
  AddImplicitCXXDestructorIfNeeded(&parser, str, tag);
  AddImplicitCXXDeductionGuides(str, tag);
  UpdateCXXAbstractStatus(str);
  AddCXXVPtrMember(&parser, str);
  AddCXXVBPtrMember(&parser, str);
  str->non_virtual_size = str->next_offset;
  LayoutCXXVirtualBaseSpecifiers(str);
  if (!str->vtables_registered) {
    RegisterCXXVTable(&parser, str);
    RegisterCXXVBTables(&parser, str);
    str->vtables_registered = true;
    SyntaxFlushPendingVPtrInitializers(str);
  }
  CXXFixupSpecialMemberTrivialityAfterLayout(str);
  FinalizeStructAlignment(str);
  TypeRecordCalculateSize(tag->type);
  CheckFlexibleArrays(&parser, str, str->is_union);
  if (CompilerIsCXX() && str != NULL && !str->is_union &&
      str->defining_template_scope_count == 0 && tag != NULL &&
      !tag->flags.invented) {
    VectorAppend(&compiler->cxx_defined_classes, str);
  }
  TypeParserDestruct(&parser);
}

static ReflectionValue* MetaSynthesisDefineAggregate(
    ReflectionValue* class_type, Vector* specs, SourceLocation location) {
  if (class_type == NULL || class_type->reflected_type == NULL ||
      !TypeIsStructOrUnion(class_type->reflected_type) ||
      class_type->reflected_type->info.struct_info == NULL) {
    return NULL;
  }
  Struct* str = class_type->reflected_type->info.struct_info;
  size_t old_member_count = str->members.length;
  bool changed = false;
  for (size_t i = 0; i < specs->length; i++) {
    ReflectionValue* spec_value = specs->value.p[i];
    if (spec_value == NULL ||
        spec_value->kind != kReflectionDataMemberDescription ||
        spec_value->data_member_spec == NULL) {
      MetaSynthesisRollbackMembers(str, old_member_count);
      return NULL;
    }
    StructMember* existing = MetaSynthesisFindMatchingMember(
        str, spec_value->data_member_spec, i);
    if (existing != NULL &&
        MetaSynthesisMemberMatchesSpec(existing, spec_value->data_member_spec)) {
      continue;
    }
    if (existing != NULL) {
      SemanticError(NULL,
                    "define_aggregate: conflicting existing data member");
      MetaSynthesisRollbackMembers(str, old_member_count);
      return NULL;
    }
    if (!MetaSynthesisApplyDataMemberSpec(str, spec_value->data_member_spec,
                                          i, location)) {
      MetaSynthesisRollbackMembers(str, old_member_count);
      return NULL;
    }
    changed = true;
  }
  if (!changed && str->meta_aggregate_complete) {
    return ReflectionCreateType(class_type->reflected_type, NULL, location);
  }
  if (!RelayoutStruct(str)) {
    MetaSynthesisRollbackMembers(str, old_member_count);
    return NULL;
  }
  ComputeCXXAggregateStatus(str);
  MetaSynthesisCompleteClassAfterAggregate(str);
  if (str->tag_symbol != NULL && str->tag_symbol->type != NULL) {
    TypeRecordCalculateSize(str->tag_symbol->type);
    str->tag_symbol->flags.is_forward_declared = false;
    str->tag_symbol->flags.is_defined = true;
  }
  str->meta_aggregate_complete = true;
  return ReflectionCreateType(class_type->reflected_type, NULL, location);
}

static Symbol* MetaSynthesisFindPromotedStatic(const char* key) {
  if (key == NULL || compiler->meta_promoted_statics.length == 0) {
    return NULL;
  }
  for (size_t i = 0; i < compiler->meta_promoted_statics.length; i++) {
    MetaPromotedStaticEntry* entry = compiler->meta_promoted_statics.value.p[i];
    if (entry != NULL && entry->key.value != NULL &&
        strcmp(entry->key.value, key) == 0) {
      return entry->symbol;
    }
  }
  return NULL;
}

static Symbol* MetaSynthesisRegisterPromotedStatic(const char* key,
                                                   Symbol* symbol) {
  MetaPromotedStaticEntry* entry = calloc(1, sizeof(*entry));
  StringInit(&entry->key, key);
  entry->symbol = symbol;
  VectorAppend(&compiler->meta_promoted_statics, entry);
  return symbol;
}

static TypeRecord* MetaSynthesisPointerTo(TypeRecord* pointee) {
  TypeRecord* pointer = NewPointerTypeRecord(kQualPlain);
  TypeRecordChain(pointer, pointee);
  TypeRecordCalculateSize(pointer);
  return pointer;
}

static TypeRecord* MetaSynthesisCallReturnType(VectorASTNode* call) {
  return call != NULL && call->left != NULL && call->left->type != NULL &&
                 TypeIsFunction(call->left->type)
             ? call->left->type->next
             : NULL;
}

static TypeRecord* MetaSynthesisDynamicSpanType(TypeRecord* element_type) {
  Namespace* std_ns = NamespaceFindStdNamespace();
  if (std_ns == NULL || element_type == NULL) {
    return NULL;
  }
  String name;
  StringInit(&name, "span");
  NamespaceInlineTagLookup lookup =
      NamespaceResolveTagInInlineSet(std_ns, &name);
  StringDestruct(&name);
  if (lookup.status != kInlineLookupUnique || lookup.tag == NULL ||
      !lookup.tag->flags.is_template) {
    return NULL;
  }
  TypeRecord* const_element = TypeRecordCopy(element_type);
  const_element->qualifiers |= kQualConst;
  Vector* args = NewVector();
  VectorAppend(args, NewTypeTemplateArgument(const_element));
  TypeRecord* result =
      TypeInstantiateClassTemplate(&compiler->syntax, lookup.tag, args);
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return result;
}

static ASTNode* MetaSynthesisAddressOfPromoted(Symbol* symbol,
                                               TypeRecord* pointee,
                                               SourceLocation location) {
  ASTNode* id = NewIdentifierASTNode(symbol, location);
  ASTNode* address = NewUnaryASTNode(
      AST_OP(address),
      MetaSynthesisPointerTo(TypeRecordCopy(pointee)), location, id);
  return address;
}

static ASTNode* MetaSynthesisPromotedStaticExpression(Symbol* symbol,
                                                    TypeRecord* pointee,
                                                    SourceLocation location) {
  if (symbol == NULL || pointee == NULL) {
    return NULL;
  }
  if (TypeIsArray(symbol->type)) {
    ASTNode* id = NewIdentifierASTNode(symbol, location);
    ASTNodeSetType(id, MetaSynthesisPointerTo(TypeRecordCopy(pointee)));
    return id;
  }
  return MetaSynthesisAddressOfPromoted(symbol, pointee, location);
}

static Symbol* MetaSynthesisPromoteStaticData(TypeRecord* type,
                                              ASTNode* initializer,
                                              const char* key,
                                              SourceLocation location) {
  Symbol* existing = MetaSynthesisFindPromotedStatic(key);
  if (existing != NULL) {
    return existing;
  }
  char name_buffer[64];
  snprintf(name_buffer, sizeof(name_buffer), "__meta_static_%zu",
           compiler->meta_promoted_statics.length);
  Symbol* symbol = NewSymbol(name_buffer, TypeRecordCopy(type), STO(static));
  symbol->flags.is_defined = true;
  symbol->flags.is_constexpr = true;
  symbol->flags.value_set = true;
  symbol->location = location;
  symbol->cxx_linkage = kCXXLinkageInternal;
  symbol->constexpr_initializer = initializer != NULL
                                      ? ASTNodeClone(initializer, IdentityCloneNode,
                                                     NULL, NULL)
                                      : NULL;
  MetaSynthesisRegisterPromotedStatic(key, symbol);
  CompilerRegisterMetaPromotedStatic(symbol, initializer);
  return symbol;
}

static bool MetaSynthesisCollectStringChars(ASTNode* node, String* out) {
  if (node == NULL || out == NULL) {
    return false;
  }
  if (node->op == AST_OP(string)) {
    String* text = ((ConstantASTNode*)node)->value.string;
    if (text != NULL && text->value != NULL) {
      StringAppendString(out, text);
      return true;
    }
    return false;
  }
  if (node->op == AST_OP(braced_init)) {
    BracedInitializerASTNode* braced = (BracedInitializerASTNode*)node;
    for (size_t i = 0; i < braced->initializers->length; i++) {
      if (!MetaSynthesisCollectStringChars(braced->initializers->value.p[i],
                                           out)) {
        return false;
      }
    }
    return true;
  }
  int64_t ch = 0;
  if (EvaluateIntegerExpression(node, &ch)) {
    StringAppendChar(out, (char)ch);
    return true;
  }
  return false;
}

static ReflectionValue* MetaSynthesisReflectConstantFromExpression(
    ASTNode* expression, SourceLocation location) {
  if (expression == NULL) {
    return NULL;
  }
  expression = AnalyzeExpression(expression);
  ReflectionValue* existing =
      SemanticReflectionValueFromExpression(expression);
  if (existing != NULL) {
    return existing;
  }
  if (expression->type != NULL && TypeIsReflection(expression->type)) {
    ConstEvalContext context;
    ConstEvalContextInit(&context);
    existing = ConstexprEvaluateReflectionExpression(&context, expression);
    ConstEvalContextDestruct(&context);
    if (existing != NULL) {
      return existing;
    }
  }
  if (expression->type != NULL && TypeIsIntegral(expression->type)) {
    int64_t value = 0;
    if (EvaluateIntegerExpression(expression, &value)) {
      return ReflectionCreateScalar(TypeRecordCopy(expression->type), value, 0.0,
                                  false, location);
    }
  }
  if (expression->type != NULL && TypeIsFloatingPoint(expression->type)) {
    double value = 0.0;
    if (expression->op == AST_OP(number)) {
      value = ((ConstantASTNode*)expression)->value.fvalue;
      return ReflectionCreateScalar(TypeRecordCopy(expression->type), 0, value,
                                    true, location);
    }
  }
  if (expression->op == AST_OP(braced_init)) {
    BracedInitializerASTNode* braced = (BracedInitializerASTNode*)expression;
    Vector values;
    VectorInit(&values);
    for (size_t i = 0; i < braced->initializers->length; i++) {
      ReflectionValue* element = MetaSynthesisReflectConstantFromExpression(
          braced->initializers->value.p[i], location);
      if (element == NULL) {
        VectorDestruct(&values);
        return NULL;
      }
      VectorAppend(&values, element);
    }
    TypeRecord* element_type = NULL;
    if (expression->type != NULL && TypeIsArray(expression->type)) {
      element_type = TypeRecordCopy(expression->type->next);
    } else if (values.length > 0) {
      ReflectionValue* first = values.value.p[0];
      if (first != NULL && first->reflected_type != NULL) {
        element_type = TypeRecordCopy(first->reflected_type);
      }
    }
    if (element_type == NULL) {
      element_type = NewTypeRecordWithSize(kTypeInt, kQualConst);
    }
    ReflectionValue* sequence = ReflectionCreateSequence(
        kReflectionValue, element_type, &values, location);
    TypeRecordDelete(element_type);
    VectorDestruct(&values);
    return sequence;
  }
  if (expression->type != NULL) {
    ASTNode* init = NewExpressionInitializerASTNode(
        ASTNodeClone(expression, IdentityCloneNode, NULL, NULL), location);
    return ReflectionCreateWithInitializer(TypeRecordCopy(expression->type),
                                           init, location);
  }
  return NULL;
}

static bool MetaSynthesisExpressionIsStringLiteralSource(ASTNode* arg) {
  if (arg == NULL) {
    return false;
  }
  if (arg->op == AST_OP(uplus) || arg->op == AST_OP(uminus)) {
    return false;
  }
  if (arg->op == AST_OP(string) || arg->op == AST_OP(string_wide)) {
    return true;
  }
  if (arg->op == AST_OP(identifier)) {
    Symbol* symbol = ((IdentifierASTNode*)arg)->symbol;
    if (CompilerSymbolIsMetaPromotedString(symbol)) {
      return true;
    }
    Symbol* target = CompilerMetaPromotedPointerTarget(symbol);
    return target != NULL && CompilerSymbolIsMetaPromotedString(target);
  }
  return false;
}

static TypeRecord* MetaSynthesisExtractTargetType(VectorASTNode* call) {
  if (call == NULL || call->left == NULL ||
      call->left->op != AST_OP(identifier)) {
    return NULL;
  }
  IdentifierASTNode* callee = (IdentifierASTNode*)call->left;
  if (callee->template_arguments == NULL ||
      callee->template_arguments->length == 0) {
    return NULL;
  }
  TemplateArgument* arg = callee->template_arguments->value.p[0];
  return MetaSynthesisResolveTemplateArgumentType(arg);
}

static void MetaSynthesisMaybeWrapSubstituted(Symbol* templ, Vector* args,
                                              bool create_substituted,
                                              ReflectionValue** out_value,
                                              SourceLocation location) {
  if (!create_substituted || out_value == NULL || *out_value == NULL ||
      templ == NULL) {
    return;
  }
  ReflectionValue* result = *out_value;
  ReflectionValue* substituted =
      ReflectionCreateSubstituted(templ, args, location);
  if (substituted == NULL) {
    return;
  }
  substituted->kind = result->kind;
  if (result->reflected_type != NULL) {
    substituted->reflected_type = TypeRecordCopy(result->reflected_type);
  }
  if (result->symbol != NULL) {
    substituted->symbol = result->symbol;
  }
  substituted->scalar_ivalue = result->scalar_ivalue;
  substituted->scalar_fvalue = result->scalar_fvalue;
  substituted->scalar_is_float = result->scalar_is_float;
  *out_value = substituted;
}

static bool MetaSynthesisTryInstantiateTemplate(Symbol* templ, Vector* args,
                                                bool quiet,
                                                bool create_substituted,
                                                ReflectionValue** out_value,
                                                SourceLocation location) {
  if (templ == NULL || args == NULL || out_value == NULL) {
    return false;
  }
  if (templ->flags.is_template && templ->type != NULL &&
      TypeIsStructOrUnion(templ->type)) {
    TypeRecord* type = quiet
                           ? TypeInstantiateClassTemplateQuiet(
                                 &compiler->syntax, templ, args)
                           : TypeInstantiateClassTemplate(&compiler->syntax,
                                                          templ, args);
    if (type == NULL) {
      return false;
    }
    *out_value = ReflectionCreateType(type, NULL, location);
    MetaSynthesisMaybeWrapSubstituted(templ, args, create_substituted,
                                      out_value, location);
    return true;
  }
  if (templ->flags.is_template && templ->type != NULL &&
      TypeIsFunction(templ->type)) {
    Symbol* function =
        TypeInstantiateFunctionTemplate(&compiler->syntax, templ, args);
    if (function == NULL) {
      return false;
    }
    *out_value = ReflectionCreateSymbol(kReflectionFunction, function, location);
    MetaSynthesisMaybeWrapSubstituted(templ, args, create_substituted,
                                      out_value, location);
    return true;
  }
  if (templ->alias_template != NULL) {
    TypeParser parser;
    TypeParserInit(&parser, compiler->syntax.lex, &compiler->syntax,
                   STO(implicit), compiler->syntax.context);
    TypeRecord* type =
        InstantiateAliasClassTemplate(&parser, templ, args);
    TypeParserDestruct(&parser);
    if (type == NULL) {
      return false;
    }
    *out_value = ReflectionCreateType(type, NULL, location);
    MetaSynthesisMaybeWrapSubstituted(templ, args, create_substituted,
                                      out_value, location);
    return true;
  }
  if (templ->variable_template != NULL) {
    if (templ->type != NULL && TypeIsIntegral(templ->type)) {
      int64_t value = 0;
      bool ok = quiet
                    ? TypeInstantiateVariableTemplateConstantQuiet(
                          &compiler->syntax, templ, args, &value)
                    : TypeInstantiateVariableTemplateConstant(
                          &compiler->syntax, templ, args, &value);
      if (!ok) {
        return false;
      }
      *out_value = ReflectionCreateScalar(TypeRecordCopy(templ->type), value, 0.0,
                                          false, location);
    } else if (templ->type != NULL && TypeIsFloatingPoint(templ->type)) {
      double value = 0.0;
      bool ok = quiet
                    ? TypeInstantiateVariableTemplateFloatingConstantQuiet(
                          &compiler->syntax, templ, args, &value)
                    : TypeInstantiateVariableTemplateFloatingConstant(
                          &compiler->syntax, templ, args, &value);
      if (!ok) {
        return false;
      }
      *out_value = ReflectionCreateScalar(TypeRecordCopy(templ->type), 0, value,
                                          true, location);
    } else {
      TypeRecord* type =
          quiet ? TypeInstantiateVariableTemplateTypeQuiet(&compiler->syntax,
                                                           templ, args)
                : TypeInstantiateVariableTemplateType(&compiler->syntax, templ,
                                                      args);
      if (type == NULL) {
        return false;
      }
      *out_value = ReflectionCreateType(type, NULL, location);
    }
    MetaSynthesisMaybeWrapSubstituted(templ, args, create_substituted,
                                      out_value, location);
    return true;
  }
  return false;
}

ASTNode* SemanticTryAnalyzeMetaSynthesisCallEarly(VectorASTNode* call) {
  Symbol* function = MetaSynthesisCallSymbol(call);
  if (function == NULL || function->name.value == NULL) {
    return NULL;
  }
  MetaSynthesisOperation operation =
      MetaSynthesisOperationForName(function->name.value);
  if (operation != kMetaSynthDataMemberSpec &&
      operation != kMetaSynthReflectObject &&
      operation != kMetaSynthReflectFunction) {
    return NULL;
  }
  return SemanticTryAnalyzeMetaSynthesisCall(call);
}

ASTNode* SemanticTryAnalyzeMetaSynthesisCall(VectorASTNode* call) {
  Symbol* function = MetaSynthesisCallSymbol(call);
  if (function == NULL) {
    return NULL;
  }
  const char* name = function->name.value;
  if (name == NULL) {
    return NULL;
  }
  MetaSynthesisOperation operation = MetaSynthesisOperationForName(name);
  if (operation == kMetaSynthUnknown) {
    return NULL;
  }
  for (size_t i = 0; call->children != NULL && i < call->children->length;
       i++) {
    if ((operation == kMetaSynthDataMemberSpec && i == 1) ||
        ((operation == kMetaSynthReflectObject ||
          operation == kMetaSynthReflectFunction) &&
         i == 0)) {
      continue;
    }
    call->children->value.p[i] =
        AnalyzeExpression(call->children->value.p[i]);
  }

  switch (operation) {
    case kMetaSynthIsDataMemberSpec: {
      if (call->children == NULL || call->children->length < 1) {
        return NULL;
      }
      ReflectionValue* value =
          MetaSynthesisEvaluateReflection(call->children->value.p[0]);
      return MetaSynthesisBool(
          value != NULL && value->kind == kReflectionDataMemberDescription,
          call->base.location);
    }
    case kMetaSynthIsStringLiteral: {
      if (call->children == NULL || call->children->length < 1) {
        return NULL;
      }
      ASTNode* arg = call->children->value.p[0];
      return MetaSynthesisBool(MetaSynthesisExpressionIsStringLiteralSource(arg),
                               call->base.location);
    }
    case kMetaSynthReflectConstant: {
      if (call->children == NULL || call->children->length < 1) {
        return NULL;
      }
      ReflectionValue* value = MetaSynthesisReflectConstantFromExpression(
          call->children->value.p[0], call->base.location);
      return value != NULL
                 ? NewReflectionConstantASTNode(value, call->base.location)
                 : NULL;
    }
    case kMetaSynthReflectObject: {
      if (call->children == NULL || call->children->length < 1) {
        return NULL;
      }
      ASTNode* arg = call->children->value.p[0];
      Symbol* symbol = MetaSynthesisReferencedSymbol(arg);
      if (symbol == NULL || symbol->type == NULL ||
          TypeIsFunction(symbol->type)) {
        SemanticError((ASTNode*)call,
                      "reflect_object requires an object expression");
        return NULL;
      }
      ReflectionValue* value =
          ReflectionCreateSymbol(kReflectionObject, symbol, call->base.location);
      return NewReflectionConstantASTNode(value, call->base.location);
    }
    case kMetaSynthReflectFunction: {
      if (call->children == NULL || call->children->length < 1) {
        return NULL;
      }
      ASTNode* arg = call->children->value.p[0];
      Symbol* symbol = MetaSynthesisReferencedSymbol(arg);
      if (symbol == NULL || symbol->type == NULL ||
          !TypeIsFunction(symbol->type)) {
        SemanticError((ASTNode*)call,
                      "reflect_function requires a function expression");
        return NULL;
      }
      ReflectionValue* value = ReflectionCreateSymbol(kReflectionFunction,
                                                        symbol,
                                                        call->base.location);
      return NewReflectionConstantASTNode(value, call->base.location);
    }
    case kMetaSynthDataMemberSpec: {
      if (call->children == NULL || call->children->length < 2) {
        return NULL;
      }
      ReflectionValue* type_value =
          MetaSynthesisEvaluateReflection(call->children->value.p[0]);
      TypeRecord* member_type =
          SemanticMaterializeReflectedType(type_value, call->base.location);
      ReflectionDataMemberSpec* spec = NULL;
      if (!MetaSynthesisParseDataMemberOptions(call->children->value.p[1],
                                               member_type, &spec,
                                               call->base.location)) {
        TypeRecordDelete(member_type);
        SemanticError((ASTNode*)call, "invalid data_member_spec arguments");
        return NULL;
      }
      TypeRecordDelete(member_type);
      ReflectionValue* value = ReflectionCreateDataMemberSpec(
          spec, call->base.location);
      return NewReflectionConstantASTNode(value, call->base.location);
    }
    case kMetaSynthDefineAggregate: {
      if (call->children == NULL || call->children->length < 2) {
        return NULL;
      }
      ReflectionValue* class_type =
          MetaSynthesisEvaluateReflection(call->children->value.p[0]);
      Vector specs;
      VectorInit(&specs);
      if (class_type == NULL ||
          !MetaSynthesisReflectionRange(call, 1, &specs)) {
        VectorDestruct(&specs);
        return NULL;
      }
      ReflectionValue* result = MetaSynthesisDefineAggregate(
          class_type, &specs, call->base.location);
      VectorDestruct(&specs);
      return result != NULL
                 ? NewReflectionConstantASTNode(result, call->base.location)
                 : NULL;
    }
    case kMetaSynthReflectConstantString:
    case kMetaSynthReflectConstantArray: {
      if (call->children == NULL || call->children->length < 1) {
        return NULL;
      }
      Vector values;
      VectorInit(&values);
      if (!MetaSynthesisReflectionRange(call, 0, &values)) {
        String chars;
        StringInit(&chars, NULL);
        if (!MetaSynthesisCollectStringChars(call->children->value.p[0],
                                             &chars)) {
          StringDestruct(&chars);
          VectorDestruct(&values);
          return NULL;
        }
        for (size_t i = 0; i < chars.length; i++) {
          ReflectionValue* ch = ReflectionCreateScalar(
              NewTypeRecordWithSize(kTypeChar, kQualConst), chars.value[i], 0.0,
              false, call->base.location);
          VectorAppend(&values, ch);
        }
        StringDestruct(&chars);
      }
      TypeRecord* element_type =
          operation == kMetaSynthReflectConstantString
              ? NewTypeRecordWithSize(kTypeChar, kQualConst)
              : (values.length > 0
                     ? ((ReflectionValue*)values.value.p[0])->reflected_type
                     : NewTypeRecordWithSize(kTypeInt, kQualConst));
      ReflectionValue* value = ReflectionCreateSequence(
          kReflectionValue, element_type, &values, call->base.location);
      VectorDestruct(&values);
      return NewReflectionConstantASTNode(value, call->base.location);
    }
    case kMetaSynthDefineStaticString: {
      if (call->children == NULL || call->children->length < 1) {
        return NULL;
      }
      String contents;
      StringInit(&contents, NULL);
      if (!MetaSynthesisCollectStringChars(call->children->value.p[0],
                                           &contents)) {
        StringDestruct(&contents);
        return NULL;
      }
      StringAppendChar(&contents, '\0');
      char key_buffer[256];
      snprintf(key_buffer, sizeof(key_buffer), "str:%zu", contents.length);
      for (size_t i = 0; i < contents.length && i < 128; i++) {
        char tmp[8];
        snprintf(tmp, sizeof(tmp), ":%02x", (unsigned char)contents.value[i]);
        strncat(key_buffer, tmp, sizeof(key_buffer) - strlen(key_buffer) - 1);
      }
      TypeRecord* array = NewBasicArrayTypeRecord(
          kQualPlain, (int)contents.length, false);
      TypeRecordChain(array, NewTypeRecordWithSize(kTypeChar, kQualConst));
      TypeRecordCalculateSize(array);
      ASTNode* init = NewStringConstantASTNode(
          NewString(contents.value != NULL ? contents.value : ""), array,
          call->base.location);
      Symbol* symbol = MetaSynthesisPromoteStaticData(
          array, init, key_buffer, call->base.location);
      StringDestruct(&contents);
      TypeRecord* pointee = array->next;
      return MetaSynthesisPromotedStaticExpression(symbol, pointee,
                                                   call->base.location);
    }
    case kMetaSynthDefineStaticArray:
    case kMetaSynthDefineStaticObject: {
      if (call->children == NULL || call->children->length < 1) {
        return NULL;
      }
      ASTNode* arg = call->children->value.p[0];
      if (arg == NULL || arg->type == NULL) {
        return NULL;
      }
      char key_buffer[128];
      snprintf(key_buffer, sizeof(key_buffer), "obj:%p", (void*)arg);
      ASTNode* promoted_initializer = arg;
      if (arg->op == AST_OP(identifier)) {
        Symbol* source = ((IdentifierASTNode*)arg)->symbol;
        ASTNode* object_initializer =
            ConstexprObjectInitializerForSymbol(source, call->base.location);
        if (object_initializer != NULL) {
          promoted_initializer = object_initializer;
        } else if (source != NULL && source->constexpr_initializer != NULL) {
          promoted_initializer = source->constexpr_initializer;
        }
      }
      ASTNode* stored_initializer =
          ASTNodeClone(promoted_initializer, IdentityCloneNode, NULL, NULL);
      if (stored_initializer->op != AST_OP(expr_init)) {
        stored_initializer = NewExpressionInitializerASTNode(
            stored_initializer, call->base.location);
      }
      Symbol* symbol = MetaSynthesisPromoteStaticData(
          arg->type, stored_initializer, key_buffer, call->base.location);
      TypeRecord* pointee = arg->type;
      if (TypeIsArray(arg->type)) {
        pointee = arg->type->next;
      }
      ASTNode* promoted = MetaSynthesisPromotedStaticExpression(
          symbol, pointee, call->base.location);
      if (operation == kMetaSynthDefineStaticObject ||
          !TypeIsArray(arg->type)) {
        return promoted;
      }
      TypeRecord* synthesized_span =
          MetaSynthesisDynamicSpanType(arg->type->next);
      TypeRecord* return_type =
          synthesized_span != NULL ? synthesized_span
                                   : MetaSynthesisCallReturnType(call);
      if (return_type == NULL) {
        return promoted;
      }
      Vector* span_args = NewVector();
      VectorAppend(span_args, promoted);
      VectorAppend(span_args,
                   NewIntConstantASTNode(
                       (int64_t)arg->type->info.array.size.fixed,
                       NewSizeTypeRecord(), call->base.location));
      ASTNode* braced =
          NewBracedInitializerASTNode(span_args, NULL, call->base.location);
      ASTNode* result =
          AnalyzeExpression(LowerCXXBracedInitToTarget(braced, return_type));
      TypeRecordDelete(synthesized_span);
      return result;
    }
    case kMetaSynthCanSubstitute:
    case kMetaSynthSubstitute: {
      if (call->children == NULL || call->children->length < 2) {
        return NULL;
      }
      ReflectionValue* templ =
          MetaSynthesisEvaluateReflection(call->children->value.p[0]);
      Vector reflections;
      VectorInit(&reflections);
      if (templ == NULL || templ->symbol == NULL ||
          !MetaSynthesisReflectionRange(call, 1, &reflections)) {
        VectorDestruct(&reflections);
        return NULL;
      }
      Vector* args = MetaSynthesisTemplateArgumentsFromRange(&reflections);
      VectorDestruct(&reflections);
      ReflectionValue* result = NULL;
      bool ok = MetaSynthesisTryInstantiateTemplate(
          templ->symbol, args, operation == kMetaSynthCanSubstitute,
          operation == kMetaSynthSubstitute, &result, call->base.location);
      VectorDeleteWithContents(args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      if (operation == kMetaSynthCanSubstitute) {
        return MetaSynthesisBool(ok, call->base.location);
      }
      return ok && result != NULL
                 ? NewReflectionConstantASTNode(result, call->base.location)
                 : NULL;
    }
    case kMetaSynthExtract: {
      if (call->children == NULL || call->children->length < 1) {
        return NULL;
      }
      ReflectionValue* value =
          MetaSynthesisEvaluateReflection(call->children->value.p[0]);
      if (value == NULL) {
        return NULL;
      }
      TypeRecord* target_type = MetaSynthesisExtractTargetType(call);
      if (target_type != NULL) {
        if (TypeIsStructOrUnion(target_type)) {
          TypeRecord* materialized =
              SemanticMaterializeReflectedType(value, call->base.location);
          if (materialized != NULL) {
            return NewReflectionConstantASTNode(
                ReflectionCreateType(materialized, NULL, call->base.location),
                call->base.location);
          }
        } else if (value->kind == kReflectionType ||
                   value->kind == kReflectionTypeAlias) {
          TypeRecord* materialized =
              SemanticMaterializeReflectedType(value, call->base.location);
          if (materialized != NULL && TypeEqual(materialized, target_type)) {
            return NewReflectionConstantASTNode(
                ReflectionCreateType(materialized, NULL, call->base.location),
                call->base.location);
          }
        } else if (value->extract_type != NULL &&
                   TypeEqual(value->extract_type, target_type)) {
          return NewReflectionConstantASTNode(
              ReflectionCreateType(TypeRecordCopy(value->extract_type), NULL,
                                   call->base.location),
              call->base.location);
        }
      }
      if (value->kind == kReflectionValue) {
        if (value->constexpr_initializer != NULL) {
          return AnalyzeExpression(value->constexpr_initializer);
        }
        if (value->reflected_type != NULL &&
            TypeIsIntegral(value->reflected_type)) {
          return NewIntConstantASTNode(
              value->scalar_ivalue,
              TypeRecordCopy(value->reflected_type), call->base.location);
        }
      }
      if (value->symbol != NULL) {
        ASTNode* identifier =
            NewIdentifierASTNode(value->symbol, call->base.location);
        if (target_type != NULL && TypeIsPointer(target_type) &&
            TypeIsFunction(value->symbol->type)) {
          return AnalyzeExpression(NewCastASTNode(
              TypeRecordCopy(target_type), call->base.location, identifier));
        }
        return identifier;
      }
      return NULL;
    }
    case kMetaSynthUnknown:
      return NULL;
  }
  return NULL;
}
